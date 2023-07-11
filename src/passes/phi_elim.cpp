#include "passes/phi_elim.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void phi_elim(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    builder.switch_function(function_name);
    phi_elim_function(function, builder);
  }
}

void phi_elim_function(FunctionPtr function, Builder& builder) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    auto next_bb = curr_bb->next;

    auto curr_instr = curr_bb->head_instruction->next;

    while (curr_instr->is_phi() && curr_instr != curr_bb->tail_instruction) {
      auto next_instr = curr_instr->next;

      auto phi = std::get<instruction::Phi>(curr_instr->kind);

      auto pred_id_set = std::set<BasicBlockID>();
      for (auto pred_id : curr_bb->pred_list) {
        pred_id_set.insert(pred_id);
      }

      auto dst = builder.context.get_operand(phi.dst_id);

      auto allocated_type = dst->type;

      auto ptr_id = builder.fetch_arbitrary_operand(
        builder.fetch_pointer_type(allocated_type)
      );

      auto alloca_instr = builder.fetch_alloca_instruction(
        ptr_id, allocated_type, std::nullopt, std::nullopt, std::nullopt
      );

      builder.prepend_instruction_to_curr_function(alloca_instr);

      for (auto [incoming_operand_id, incoming_bb_id] : phi.incoming_list) {
        auto incoming_bb = builder.context.get_basic_block(incoming_bb_id);

        if (!pred_id_set.count(incoming_bb_id)) {
          continue;
        }

        auto br_instr = incoming_bb->tail_instruction->prev.lock();
        builder.set_curr_basic_block(incoming_bb);
        auto store_instr = builder.fetch_store_instruction(
          incoming_operand_id, ptr_id, std::nullopt
        );
        br_instr->insert_prev(store_instr);
        builder.set_curr_basic_block(curr_bb);
      }

      auto load_instr =
        builder.fetch_load_instruction(phi.dst_id, ptr_id, std::nullopt);

      curr_instr->insert_prev(load_instr);

      curr_instr->remove(builder.context);

      curr_instr = next_instr;
    }

    curr_bb = next_bb;
  }
}

}  // namespace ir
}  // namespace syc