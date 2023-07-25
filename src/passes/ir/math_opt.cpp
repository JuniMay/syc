#include "passes/ir/math_opt.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void math_opt(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    math_opt_function(function, builder);
  }
}

void math_opt_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    math_opt_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void math_opt_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;
  builder.set_curr_basic_block(basic_block);
  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;
    // optimize add chain to mul
    // %b = add %a, %x
    // %c = add %b, %x
    // %d = add %c, %x
    // ->
    // %y = mul %x, 3
    // %d = add %a, %y
    auto curr_maybe_binary = curr_instruction->as<Binary>();
    auto next_maybe_binary = next_instruction->as<Binary>();
    OperandID src_operand_id = 0;
    OperandID dst_operand_id = 0;
    OperandID adder_operand_id = 0;
    int count = 0;
    while (curr_maybe_binary.has_value() && next_maybe_binary.has_value()) {
      auto curr_binary = curr_maybe_binary.value();
      auto next_binary = next_maybe_binary.value();
      auto curr_op = curr_binary.op;
      auto next_op = next_binary.op;
      auto curr_dst = builder.context.get_operand(curr_binary.dst_id);
      auto curr_lhs = builder.context.get_operand(curr_binary.lhs_id);
      auto curr_rhs = builder.context.get_operand(curr_binary.rhs_id);
      auto next_dst = builder.context.get_operand(next_binary.dst_id);
      auto next_lhs = builder.context.get_operand(next_binary.lhs_id);
      auto next_rhs = builder.context.get_operand(next_binary.rhs_id);
      if (curr_op == BinaryOp::Add && 
      curr_lhs->is_arbitrary() && 
      curr_rhs->is_arbitrary() &&
      next_op == BinaryOp::Add &&
      next_lhs->is_arbitrary() &&
      next_rhs->is_arbitrary() &&
      curr_dst->id == next_lhs->id &&
      curr_rhs->id == next_rhs->id) {
        if (src_operand_id == 0) {
          src_operand_id = curr_lhs->id;
          adder_operand_id = curr_rhs->id;
        }
        dst_operand_id = next_dst->id;
        count++;
        curr_instruction->remove(builder.context);
      } else {
        break;
      }
      curr_instruction = next_instruction;
      next_instruction = next_instruction->next;
      curr_maybe_binary = curr_instruction->as<Binary>();
      next_maybe_binary = next_instruction->as<Binary>();
    }
    if (count > 0) {
      auto sum_operand_id = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
      curr_instruction->insert_next(builder.fetch_binary_instruction(
        BinaryOp::Add,
        dst_operand_id,
        src_operand_id,
        sum_operand_id
      ));
      curr_instruction->insert_next(builder.fetch_binary_instruction(
        BinaryOp::Mul,
        sum_operand_id,
        adder_operand_id,
        builder.fetch_constant_operand(builder.fetch_i32_type(), count + 1)
      ));
      curr_instruction->remove(builder.context);
    }
    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc