#include "passes/cse.h"

namespace syc {
namespace ir {

void local_cse(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    local_cse_function(function, builder);
  }
}

void local_cse_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    local_cse_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void local_cse_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  std::map<std::tuple<instruction::BinaryOp, OperandID, OperandID>, OperandID>
    binary_expr_map;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    if (curr_instruction->is_binary()) {
      auto binary = std::get<instruction::Binary>(curr_instruction->kind);
      auto key = std::make_tuple(binary.op, binary.lhs_id, binary.rhs_id);
      if (binary_expr_map.count(key)) {
        auto existed_operand_id = binary_expr_map[key];
        auto dst = builder.context.get_operand(binary.dst_id);
        auto use_id_list_copy = dst->use_id_list;
        for (auto use_id : use_id_list_copy) {
          auto use_instruction = builder.context.get_instruction(use_id);
          use_instruction->replace_operand(
            binary.dst_id, existed_operand_id, builder.context
          );
        }
        curr_instruction->remove(builder.context);
      } else {
        binary_expr_map[key] = binary.dst_id;
      }
    }
    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc