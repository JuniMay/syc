#include "passes/unused_elim.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void unused_elim(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    unused_elim_function(function, builder);
  }
}

void unused_elim_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    unused_elim_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void unused_elim_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  auto curr_instruction = basic_block->head_instruction->next;

  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;
    auto dst_operand = curr_instruction->get_dest_operand();
    if(dst_operand.has_value()) {
        auto dst_operand_id = dst_operand.value();
        if (builder.context.get_operand(dst_operand_id)->use_id_list.size() == 0) {
            curr_instruction->remove(builder.context);
        }
    }
    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc