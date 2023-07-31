#include "passes__ir__global2local.h"

namespace syc {
namespace ir {

void global2local(Builder& builder) {
  auto global_list_copy = builder.context.global_list;

  for (auto operand_id : global_list_copy) {
    auto operand = builder.context.get_operand(operand_id);

    auto type = operand->type;

    if (type->as<type::Array>().has_value()) {
      // TODO: Array to local
      continue;
    }

    std::set<std::string> used_function_set;

    for (auto use_id : operand->use_id_list) {
      auto instr = builder.context.get_instruction(use_id);
      auto bb = builder.context.get_basic_block(instr->parent_block_id);
      used_function_set.insert(bb->parent_function_name);
    }

    if (used_function_set.size() > 1) {
      continue;
    }

    if (used_function_set.size() == 0) {
      builder.context.global_list.erase(
        std::remove(
          builder.context.global_list.begin(),
          builder.context.global_list.end(), operand_id
        ),
        builder.context.global_list.end()
      );

      continue;
    }

    // Only in main function after inlining
    if (*used_function_set.begin() != "main") {
      continue;
    }

    builder.switch_function(*used_function_set.begin());

    // Get type for global will return a pointer type.
    auto ptr_id = builder.fetch_arbitrary_operand(operand->get_type());

    auto alloca_instr = builder.fetch_alloca_instruction(
      ptr_id, operand->type, std::nullopt, std::nullopt, std::nullopt, false
    );

    builder.prepend_instruction_to_curr_function(alloca_instr);

    auto global = std::get<operand::Global>(operand->kind);

    // Shall be a constant
    auto init_operand = builder.context.get_operand(global.init);

    auto store_instr =
      builder.fetch_store_instruction(init_operand->id, ptr_id, std::nullopt);

    auto entry_bb = builder.curr_function->head_basic_block->next;
    auto curr_instr = entry_bb->head_instruction->next;
    while (curr_instr != entry_bb->tail_instruction && curr_instr->is_alloca()
    ) {
      curr_instr = curr_instr->next;
    }

    curr_instr->insert_prev(store_instr);

    // Replace all uses of the global variable with the local variable
    auto use_id_list_copy = operand->use_id_list;
    for (auto use_id : use_id_list_copy) {
      auto instr = builder.context.get_instruction(use_id);
      instr->replace_operand(operand->id, ptr_id, builder.context);
    }

    builder.context.global_list.erase(
      std::remove(
        builder.context.global_list.begin(), builder.context.global_list.end(),
        operand_id
      ),
      builder.context.global_list.end()
    );
  }
}

}  // namespace ir
}  // namespace syc