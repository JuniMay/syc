#include "passes__ir__strength_reduce.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void strength_reduce(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    strength_reduce_function(function, builder);
  }
}

void strength_reduce_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    strength_reduce_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void strength_reduce_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;
  builder.set_curr_basic_block(basic_block);

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    auto maybe_binary = curr_instruction->as<Binary>();

    if (maybe_binary.has_value()) {
      auto curr_kind = maybe_binary.value();
      auto curr_op = curr_kind.op;
      auto curr_dst = builder.context.get_operand(curr_kind.dst_id);
      auto curr_lhs = builder.context.get_operand(curr_kind.lhs_id);
      auto curr_rhs = builder.context.get_operand(curr_kind.rhs_id);

      if (curr_op == BinaryOp::Mul 
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value == 0) {
          // mul lhs, 0 -> make all uses of dst to 0
          auto use_id_list_copy = curr_dst->use_id_list;
          auto zero =
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)0);
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(curr_dst->id, zero, builder.context);
          }
          curr_instruction->remove(builder.context);
        } else if (constant_value > 1 && (constant_value & (constant_value - 1)) == 0) {
          // dest = mul lhs, 2^k -> dest = shl lhs, k
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::Shl, curr_dst->id, curr_lhs->id,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)log2(constant_value)
            )
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        }
        // TODO: maybe optimize mul lhs, 2^k + 1
      } else if (curr_op == BinaryOp::SDiv
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value == 1) {
          // div lhs 1, make all uses of dst to lhs
          auto use_id_list_copy = curr_dst->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(
              curr_dst->id, curr_lhs->id, builder.context
            );
          }
          curr_instruction->remove(builder.context);
        } else if (constant_value == -1) {
          // div lhs, -1 -> dest = sub 0, lhs
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::Sub, curr_dst->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)0),
            curr_lhs->id
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        } else if (constant_value == 2) {
          // dest = div lhs, 2
          // ->
          // tmp = srl lhs, 31
          // dest = add tmp, lhs
          // dest = sra dest, 1
          auto tmp0 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp1 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::AShr, curr_dst->id, tmp1,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)1)
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::Add, tmp1, tmp0, curr_lhs->id
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::LShr, tmp0, curr_lhs->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)31)
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        } else if (constant_value > 2 && (constant_value & (constant_value - 1)) == 0) {
          // dest = div lhs, 2^k
          // ->
          // t0 = sra lhs, 31
          // t1 = srl t0, 32 - k
          // t2 = add lhs, t1
          // dest = sra t2, k
          auto tmp0 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp1 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp2 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::AShr, curr_dst->id, tmp2,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)log2(constant_value)
            )
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::Add, tmp2, tmp1, curr_lhs->id
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::LShr, tmp1, tmp0,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)(32 - log2(constant_value))
            )
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::AShr, tmp0, curr_lhs->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)31)
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        }
      } else if (curr_op == BinaryOp::SRem
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        // dest = srem lhs, 2^k
        // ->
        // t0 = sra lhs, 31
        // t1 = srl t0, 32 - k
        // t2 = add lhs, t1
        // t3 = and t2, 2^k - 1
        // dest = sub t3, t1
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value > 1 && (constant_value & (constant_value - 1)) == 0) {
          auto tmp0 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp1 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp2 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp3 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::Sub, curr_dst->id, tmp3, tmp1
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::And, tmp3, tmp2,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)constant_value - 1
            )
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::Add, tmp2, tmp1, curr_lhs->id
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::LShr, tmp1, tmp0,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)(32 - log2(constant_value))
            )
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            BinaryOp::AShr, tmp0, curr_lhs->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)31)
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        }
      }
    }

    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc