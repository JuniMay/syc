#include "passes/asm/peephole.h"
#include "backend/basic_block.h"
#include "backend/builder.h"
#include "backend/context.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "ir/codegen.h"

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

      if (rd->is_vreg() && imm->is_zero() && (op == BinaryImm::ADDI || op == BinaryImm::ADDIW)) {
        if (rd->def_id_list.size() == 1 && (rs->def_id_list.size() == 1 || rs->is_sp())) {
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
      const auto& kind = maybe_lui.value();
      auto rd = builder.context.get_operand(kind.rd_id);
      auto imm = builder.context.get_operand(kind.imm_id);

      if (rd->is_vreg() && imm->is_zero() && rd->def_id_list.size() == 1) {
        // remove: lui v0, 0
        // make all the uses of v0 use zero instead
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
        if (rd->def_id_list.size() == 1 && (rs2->def_id_list.size() == 1 || rs2->is_sp())) {
          auto use_id_list_copy = rd->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(rd->id, rs2->id, builder.context);
          }
          curr_instruction->remove(builder.context);
        }
      } else if (rd->is_vreg() && rs2->is_zero() && (op == Binary::ADD || op == Binary::ADDW || op == Binary::SUB || op == Binary::SUBW)) {
        if (rd->def_id_list.size() == 1 && (rs1->def_id_list.size() == 1 || rs1->is_sp())) {
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

    auto maybe_li = curr_instruction->as<Li>();

    if (maybe_li.has_value()) {
      auto next1_instr = curr_instruction->next;
      auto next2_instr = next1_instr->next;

      //          li v0, imm
      //          li v1, imm
      // Replace: mul v2, v0, v1
      //      To: li v2, imm * imm
      auto maybe_next1_li = next1_instr->as<Li>();
      auto maybe_next2_mul = next2_instr->as<Binary>();
      bool match0 = maybe_next1_li.has_value() && maybe_next2_mul.has_value() &&
                    maybe_next2_mul->op == Binary::MUL &&
                    ((maybe_next2_mul->rs1_id == maybe_li->rd_id &&
                      maybe_next2_mul->rs2_id == maybe_next1_li->rd_id) ||
                     (maybe_next2_mul->rs2_id == maybe_li->rd_id &&
                      maybe_next2_mul->rs1_id == maybe_next1_li->rd_id));

      //           li v0, imm (if i type)
      //  Replace: add v2, v1, v0
      //       To: addi v2, v1, imm
      auto maybe_next1_add = next1_instr->as<Binary>();
      bool match1 = maybe_next1_add.has_value() &&
                    maybe_next1_add->op == Binary::ADD &&
                    maybe_next1_add->rs2_id == maybe_li->rd_id;

      //           li v0, 2^k
      //  Replace: mul v2, v1, v0
      //      To: slli v2, v1, k
      auto maybe_next1_mul = next1_instr->as<Binary>();
      bool match2 = maybe_next1_mul.has_value() &&
                    maybe_next1_mul->op == Binary::MUL &&
                    maybe_next1_mul->rs2_id == maybe_li->rd_id;

      if (match0) {
        auto mul_dst = builder.context.get_operand(maybe_next2_mul->rd_id);
        if (mul_dst->is_vreg()) {
          auto imm0_operand = builder.context.get_operand(maybe_li->imm_id);
          auto imm1_operand =
            builder.context.get_operand(maybe_next1_li->imm_id);

          auto imm0 =
            std::get<int32_t>(std::get<Immediate>(imm0_operand->kind).value);
          auto imm1 =
            std::get<int32_t>(std::get<Immediate>(imm1_operand->kind).value);

          auto new_imm = imm0 * imm1;

          auto new_imm_id = builder.fetch_immediate(imm0 * imm1);

          builder.set_curr_basic_block(basic_block);
          auto new_li_instr =
            builder.fetch_li_instruction(maybe_next2_mul->rd_id, new_imm_id);
          next1_instr->insert_next(new_li_instr);

          // Remove mul
          next2_instr->remove(builder.context);
        }
      } else if (match1) {
        auto add_dst = builder.context.get_operand(maybe_next1_add->rd_id);
        if (add_dst->is_vreg()) {
          auto imm_operand = builder.context.get_operand(maybe_li->imm_id);
          auto imm =
            std::get<int32_t>(std::get<Immediate>(imm_operand->kind).value);

          if (check_itype_immediate(imm)) {
            auto new_imm_id = builder.fetch_immediate(imm);
            builder.set_curr_basic_block(basic_block);
            auto addi_instr = builder.fetch_binary_imm_instruction(
              BinaryImm::ADDI, maybe_next1_add->rd_id, maybe_next1_add->rs1_id,
              new_imm_id
            );
            curr_instruction->insert_next(addi_instr);
            next1_instr->remove(builder.context);
            next_instruction = addi_instr;
          }
        }
      } else if (match2) {
        auto mul_dst = builder.context.get_operand(maybe_next1_mul->rd_id);
        if (mul_dst->is_vreg()) {
          auto imm_operand = builder.context.get_operand(maybe_li->imm_id);
          auto imm =
            std::get<int32_t>(std::get<Immediate>(imm_operand->kind).value);

          int log2_imm = log2(imm);

          if (pow(2, log2_imm) == imm && check_itype_immediate(log2_imm)) {
            auto new_imm_id = builder.fetch_immediate(log2_imm);
            builder.set_curr_basic_block(basic_block);
            auto slli_instr = builder.fetch_binary_imm_instruction(
              BinaryImm::SLLI, maybe_next1_mul->rd_id, maybe_next1_mul->rs1_id,
              new_imm_id
            );
            curr_instruction->insert_next(slli_instr);
            next1_instr->remove(builder.context);
            next_instruction = slli_instr;
          }
        }
      }
    }

    auto maybe_float_binary = curr_instruction->as<FloatBinary>();

    if (maybe_float_binary.has_value()) {
      const auto& kind = maybe_float_binary.value();
      auto op = kind.op;
      auto rd = builder.context.get_operand(kind.rd_id);
      auto rs1 = builder.context.get_operand(kind.rs1_id);
      auto rs2 = builder.context.get_operand(kind.rs2_id);

      if (rd->is_vreg() && rs1->id == rs2->id && op == FloatBinary::FSGNJ) {
        // remove: fsgnj v0, v1, v1
        // make all the uses of v0 use v1 instead
        if (rd->def_id_list.size() == 1 && rs1->def_id_list.size() == 1) {
          auto use_id_list_copy = rd->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(rd->id, rs1->id, builder.context);
          }
          curr_instruction->remove(builder.context);
        }
      } else if (op == FloatBinary::FMUL && 
                 next_instruction != basic_block->tail_instruction &&
                 rd->use_id_list.size() == 1) {
        // fmul v0, v1, v2
        // fadd v3, v4, v0
        // ->
        // fmul v0, v1, v2 (will be removed by dce)
        // fmadd v3, v1, v2, v4
        auto temp_instruction = next_instruction->next;
        auto maybe_next_float_binary = next_instruction->as<FloatBinary>();
        if (maybe_next_float_binary.has_value()) {
          auto next_op = maybe_next_float_binary->op;
          auto next_rd =
            builder.context.get_operand(maybe_next_float_binary->rd_id);
          auto next_rs1 =
            builder.context.get_operand(maybe_next_float_binary->rs1_id);
          auto next_rs2 =
            builder.context.get_operand(maybe_next_float_binary->rs2_id);

          if (maybe_next_float_binary->op == FloatBinary::FADD) {
            if (next_rs1->id == rd->id) {
              builder.set_curr_basic_block(basic_block);
              auto new_instruction = builder.fetch_float_mul_add_instruction(
                FloatMulAdd::FMADD, FloatMulAdd::Fmt::S, next_rd->id, rs1->id,
                rs2->id, next_rs2->id
              );
              curr_instruction->insert_next(new_instruction);
              next_instruction->remove(builder.context);
              next_instruction = new_instruction;
            } else if (next_rs2->id == rd->id) {
              builder.set_curr_basic_block(basic_block);
              auto new_instruction = builder.fetch_float_mul_add_instruction(
                FloatMulAdd::FMADD, FloatMulAdd::Fmt::S, next_rd->id, rs1->id,
                rs2->id, next_rs1->id
              );
              curr_instruction->insert_next(new_instruction);
              next_instruction->remove(builder.context);
              next_instruction = new_instruction;
            }
          } else if (maybe_next_float_binary->op == FloatBinary::FSUB) {
            if (next_rs1->id == rd->id) {
              builder.set_curr_basic_block(basic_block);
              auto new_instruction = builder.fetch_float_mul_add_instruction(
                FloatMulAdd::FMSUB, FloatMulAdd::Fmt::S, next_rd->id, rs1->id,
                rs2->id, next_rs2->id
              );
              curr_instruction->insert_next(new_instruction);
              next_instruction->remove(builder.context);
              next_instruction = new_instruction;
            }
          }
        }
      }
    }

    curr_instruction = next_instruction;
  }
}

}  // namespace backend
}  // namespace syc