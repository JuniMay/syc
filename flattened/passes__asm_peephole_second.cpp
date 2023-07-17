#include "passes__asm_peephole_second.h"
#include "backend__basic_block.h"
#include "backend__builder.h"
#include "backend__context.h"
#include "backend__function.h"
#include "backend__instruction.h"
#include "backend__operand.h"

namespace syc {
namespace backend {

void peephole_second(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    peephole_second_function(function, builder);
  }
}

void peephole_second_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    peephole_second_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void peephole_second_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    if (std::holds_alternative<instruction::J>(curr_instruction->kind)) {
      // remove: b label
      //         label:
      const auto& kind = std::get<instruction::J>(curr_instruction->kind);
      auto next_bb_id =
        builder.context.get_basic_block(curr_instruction->parent_block_id)
          ->next->id;
      if (kind.block_id == next_bb_id) {
        curr_instruction->remove(builder.context);
      }
    }

    curr_instruction = next_instruction;
  }
}

}  // namespace backend
}  // namespace syc