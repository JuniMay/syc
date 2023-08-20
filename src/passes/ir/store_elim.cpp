#include "passes/ir/store_elim.h"
#include "ir/function.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"

namespace syc {
namespace ir {

void store_elim(Builder& builder) {
  bool has_load = false;
  for (auto& [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    for (auto bb = function->head_basic_block->next; bb != function->tail_basic_block; bb = bb->next) {
      for (auto instr = bb->head_instruction->next; instr != bb->tail_instruction; instr = instr->next) {
        if (instr->is_load()) {
          has_load = true;
          break;
        }
      }
    }
    if (!has_load) {
      // Remove all stores
      for (auto bb = function->head_basic_block->next; bb != function->tail_basic_block; bb = bb->next) {
        auto instr = bb->head_instruction->next;
        while (instr != bb->tail_instruction) {
          auto next = instr->next;
          if (instr->is_store()) {
            instr->remove(builder.context);
          }
          instr = next;
        }
      }
    }


  }
}
}
}