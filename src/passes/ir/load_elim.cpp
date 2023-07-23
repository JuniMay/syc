#include "passes/ir/load_elim.h"

namespace syc {
namespace ir {

void load_elim(Builder& builder) {
  for (auto& [function_name, function] : builder.context.function_table) {
    load_elim_function(function, builder);
  }
}

void load_elim_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    load_elim_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void load_elim_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  std::map<OperandID, OperandID> loaded_id_map;
  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    if (curr_instruction->is_load()) {
      auto load = std::get<instruction::Load>(curr_instruction->kind);
      auto dst = builder.context.get_operand(load.dst_id);
      auto ptr = builder.context.get_operand(load.ptr_id);

      if (loaded_id_map.count(ptr->id)) {
        auto loaded_dst = builder.context.get_operand(loaded_id_map[ptr->id]);
        auto use_id_list_copy = dst->use_id_list;
        for (auto use_id : use_id_list_copy) {
          auto use_instruction = builder.context.get_instruction(use_id);
          use_instruction->replace_operand(
            dst->id, loaded_dst->id, builder.context
          );
        }
        curr_instruction->remove(builder.context);
      } else {
        loaded_id_map[ptr->id] = dst->id;
      }

    } else if (curr_instruction->is_store()) {
      auto store = std::get<instruction::Store>(curr_instruction->kind);
      auto ptr = builder.context.get_operand(store.ptr_id);
      auto value = builder.context.get_operand(store.value_id);

      loaded_id_map.erase(ptr->id);
    }

    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc