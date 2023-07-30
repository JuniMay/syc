#include "passes/asm/peephole.h"
#include "backend/basic_block.h"
#include "backend/builder.h"
#include "backend/context.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"

namespace syc {
namespace backend {

void peephole(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    peephole_function(function, builder);
  }
}

void peephole_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    peephole_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void peephole_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;

  // TODO: reaching definition analysis

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    auto maybe_binary_imm = curr_instruction->as<BinaryImm>();

    if (maybe_binary_imm.has_value()) {
      const auto& kind = std::get<BinaryImm>(curr_instruction->kind);
      auto op = kind.op;
      auto rd = builder.context.get_operand(kind.rd_id);
      auto rs = builder.context.get_operand(kind.rs_id);
      auto imm = builder.context.get_operand(kind.imm_id);

      if (rd->is_vreg() && rs->is_vreg() && imm->is_zero() && (op == BinaryImm::ADDI || op == BinaryImm::ADDIW)) {
        if (rd->def_id_list.size() == 1 && rs->def_id_list.size() == 1) {
          // remove: addi(w) v0, v1, 0
          // make all the uses of v0 use v1 instead
          auto use_id_list_copy = rd->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(rd->id, rs->id, builder.context);
          }

          curr_instruction->remove(builder.context);
        }
      } else if (rd->is_vreg() && rs->is_sp() && op == BinaryImm::ADDI) {
        // remove unnecessary offset calculation
        if (next_instruction->as<Load>().has_value()) {
          auto next_kind = next_instruction->as<Load>().value();

          auto load_rd = builder.context.get_operand(next_kind.rd_id);
          auto load_rs = builder.context.get_operand(next_kind.rs_id);
          auto load_imm = builder.context.get_operand(next_kind.imm_id);

          if (load_imm->is_zero() && *load_rs == *rd) {
            // replace base register
            next_instruction->replace_operand(rd->id, rs->id, builder.context);
            // replace immediate
            next_instruction->replace_operand(
              load_imm->id, imm->id, builder.context
            );

            // remove current instruction
            if (rd->use_id_list.size() == 0) {
              curr_instruction->remove(builder.context);
            }
          }

        } else if (next_instruction->as<Store>().has_value()) {
          auto next_kind = next_instruction->as<Store>().value();

          auto store_rs1 = builder.context.get_operand(next_kind.rs1_id);
          auto store_rs2 = builder.context.get_operand(next_kind.rs2_id);
          auto store_imm = builder.context.get_operand(next_kind.imm_id);

          if (store_imm->is_zero() && *store_rs1 == *rd) {
            // replace base register
            next_instruction->replace_operand(rd->id, rs->id, builder.context);
            // replace immediate
            next_instruction->replace_operand(
              store_imm->id, imm->id, builder.context
            );

            // remove current instruction
            if (rd->use_id_list.size() == 0) {
              curr_instruction->remove(builder.context);
            }
          }
        }
      }
    }

    auto maybe_lui = curr_instruction->as<Lui>();

    if (maybe_lui.has_value()) {
      // remove: lui v0, 0
      // make all the uses of v0 use zero instead
      const auto& kind = maybe_lui.value();
      auto rd = builder.context.get_operand(kind.rd_id);
      auto imm = builder.context.get_operand(kind.imm_id);

      if (rd->is_vreg() && imm->is_zero() && rd->def_id_list.size() == 1) {
        auto use_id_list_copy = rd->use_id_list;
        for (auto use_instruction_id : use_id_list_copy) {
          auto instruction =
            builder.context.get_instruction(use_instruction_id);
          instruction->replace_operand(
            rd->id, builder.fetch_register(Register{GeneralRegister::Zero}),
            builder.context
          );
        }
        curr_instruction->remove(builder.context);
      }
    }

    auto maybe_binary = curr_instruction->as<Binary>();

    if (maybe_binary.has_value()) {
      // remove: mul v0, zero, v1
      //         mul v0, v1, zero
      // remove: add v0, zero, v1
      //         add/sub(w) v0, v1, zero
      const auto& kind = maybe_binary.value();
      auto op = kind.op;
      auto rd = builder.context.get_operand(kind.rd_id);
      auto rs1 = builder.context.get_operand(kind.rs1_id);
      auto rs2 = builder.context.get_operand(kind.rs2_id);

      if (rd->is_vreg() && (rs1->is_zero() || rs2->is_zero()) && (op == Binary::MUL || op == Binary::MULW)) {
        if (rd->def_id_list.size() == 1) {
          auto use_id_list_copy = rd->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(
              rd->id, builder.fetch_register(Register{GeneralRegister::Zero}),
              builder.context
            );
          }
          curr_instruction->remove(builder.context);
        }
      } else if (rd->is_vreg() && rs1->is_zero() && (op == Binary::ADD || op == Binary::ADDW)) {
        if (rd->def_id_list.size() == 1 && rs2->def_id_list.size() == 1) {
          auto use_id_list_copy = rd->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(rd->id, rs2->id, builder.context);
          }
          curr_instruction->remove(builder.context);
        }
      } else if (rd->is_vreg() && rs2->is_zero() && (op == Binary::ADD || op == Binary::ADDW || op == Binary::SUB || op == Binary::SUBW)) {
        if (rd->def_id_list.size() == 1 && rs1->def_id_list.size() == 1) {
          auto use_id_list_copy = rd->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(rd->id, rs1->id, builder.context);
          }
          curr_instruction->remove(builder.context);
        }
      }
    }

    curr_instruction = next_instruction;
  }
}

}  // namespace backend
}  // namespace syc