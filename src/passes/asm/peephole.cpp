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
  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    if (std::holds_alternative<instruction::BinaryImm>(curr_instruction->kind
        )) {
      const auto& kind =
        std::get<instruction::BinaryImm>(curr_instruction->kind);
      auto op = kind.op;
      auto rd = builder.context.get_operand(kind.rd_id);
      auto rs = builder.context.get_operand(kind.rs_id);
      auto imm = builder.context.get_operand(kind.imm_id);

      if (rd->is_vreg() && rs->is_vreg() && imm->is_zero() && (op == instruction::BinaryImm::ADDI || op == instruction::BinaryImm::ADDIW)) {
        if (rd->def_id_list.size() == 1) {
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
      } else if (rd->is_vreg() && rs->is_sp() && op == instruction::BinaryImm::ADDI) {
        // remove unnecessary offset calculation
        if (std::holds_alternative<instruction::Load>(next_instruction->kind)) {
          auto next_kind = std::get<instruction::Load>(next_instruction->kind);

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
            curr_instruction->remove(builder.context);
          }

        } else if (std::holds_alternative<instruction::Store>(
                     next_instruction->kind
                   )) {
          auto next_kind = std::get<instruction::Store>(next_instruction->kind);

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
            curr_instruction->remove(builder.context);
          }
        }
      }
    }

    if (std::holds_alternative<instruction::Lui>(curr_instruction->kind)) {
      // remove: lui v0, 0
      // make all the uses of v0 use zero instead
      const auto& kind = std::get<instruction::Lui>(curr_instruction->kind);
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

    if (std::holds_alternative<instruction::Binary>(curr_instruction->kind)) {
      // remove: mul v0, zero, v1
      //         mul v0, v1, zero
      // remove: add/sub(w) v0, zero, v1
      //         add/sub(w) v0, v1, zero
      const auto& kind = std::get<instruction::Binary>(curr_instruction->kind);
      auto op = kind.op;
      auto rd = builder.context.get_operand(kind.rd_id);
      auto rs1 = builder.context.get_operand(kind.rs1_id);
      auto rs2 = builder.context.get_operand(kind.rs2_id);

      if (rd->is_vreg() && (rs1->is_zero() || rs2->is_zero()) && (op == instruction::Binary::MUL || op == instruction::Binary::MULW)) {
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
      } else if (rd->is_vreg() && rs1->is_zero() && (op == instruction::Binary::ADD || op == instruction::Binary::ADDW)) {
        if (rd->use_id_list.size() == 1) {
          auto use_id_list_copy = rd->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(rd->id, rs2->id, builder.context);
          }
          curr_instruction->remove(builder.context);
        }
      } else if (rd->is_vreg() && rs2->is_zero() && (op == instruction::Binary::ADD || op == instruction::Binary::ADDW)) {
        if (rd->use_id_list.size() == 1) {
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