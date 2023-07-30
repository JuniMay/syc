#include "passes/ir/dce.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void dce(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    bool changed = true;
    while (changed) {
      changed = dce_function(function, builder);
    }
  }
}

bool dce_function(FunctionPtr function, Builder& builder) {
  bool changed = false;
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    changed = dce_basic_block(curr_basic_block, builder) || changed;
    curr_basic_block = curr_basic_block->next;
  }
  return changed;
}

bool dce_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  bool changed = false;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;
    auto maybe_dst_id = curr_instruction->maybe_def_id;
    if (maybe_dst_id.has_value()) {
      auto dst_operand_id = maybe_dst_id.value();
      if (builder.context.get_operand(dst_operand_id)->use_id_list.size() == 0 
          && !curr_instruction->is_call()) {
        curr_instruction->remove(builder.context);
        changed = true;
      }
    }
    curr_instruction = next_instruction;
  }

  return changed;
}

}  // namespace ir
}  // namespace syc