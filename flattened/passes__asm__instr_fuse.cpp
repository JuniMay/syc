#include "passes__asm__instr_fuse.h"
#include "backend__basic_block.h"
#include "backend__builder.h"
#include "backend__context.h"
#include "backend__function.h"
#include "backend__instruction.h"
#include "backend__operand.h"
#include "ir__codegen.h"

namespace syc {
namespace backend {

void instr_fuse(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    instr_fuse_function(function, builder);
  }
}

void instr_fuse_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    instr_fuse_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void instr_fuse_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

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