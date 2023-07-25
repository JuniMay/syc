#include "passes/asm/peephole_second.h"
#include "backend/basic_block.h"
#include "backend/builder.h"
#include "backend/context.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"

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
  using namespace instruction;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    auto maybe_j = curr_instruction->as<J>();
    if (maybe_j.has_value()) {
      // remove: b label
      //         label:
      const auto& kind = maybe_j.value();
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