#include "passes__ir__straighten.h"

namespace syc {
namespace ir {

void straighten(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    straighten_function(function, builder);
  }
}

void straighten_function(FunctionPtr function, Builder& builder) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block &&
         curr_bb->next != function->tail_basic_block) {
    auto next_bb = curr_bb->next;

    // Policy: fuse blocks that has one succ and the succ has one pred
    bool maybe_fuse = true;

    if (curr_bb->succ_list.size() == 1) {
      auto succ_bb = builder.context.get_basic_block(curr_bb->succ_list[0]);
      // TODO: For tail block, fuse forward
      if (succ_bb->pred_list.size() != 1 || succ_bb->next == function->tail_basic_block) {
        maybe_fuse = false;
      }
    } else {
      maybe_fuse = false;
    }

    if (!maybe_fuse) {
      curr_bb = next_bb;
      continue;
    }

    auto succ_bb = builder.context.get_basic_block(curr_bb->succ_list[0]);

    if (succ_bb->head_instruction->next->is_phi()) {
      curr_bb = next_bb;
      continue;
    }

    // Remove branch in curr_bb
    auto branch_instr = curr_bb->tail_instruction->prev.lock();
    branch_instr->remove(builder.context);

    // Append succ_bb's instructions to curr_bb
    builder.set_curr_basic_block(curr_bb);
    auto curr_instr = succ_bb->head_instruction->next;
    while (curr_instr != succ_bb->tail_instruction) {
      auto next_instr = curr_instr->next;

      curr_instr->raw_remove();
      builder.append_instruction(curr_instr);

      curr_instr = next_instr;
    }

    curr_bb->remove_succ(succ_bb->id);

    // Modify curr_bb's succ
    for (auto succ_id : succ_bb->succ_list) {
      curr_bb->add_succ(succ_id);
      auto succ_succ_bb = builder.context.get_basic_block(succ_id);
      succ_succ_bb->remove_pred(succ_bb->id);
      succ_succ_bb->add_pred(curr_bb->id);
    }

    // Modify phi instructions
    auto use_id_list_copy = succ_bb->use_id_list;
    for (auto use_id : use_id_list_copy) {
      auto use = builder.context.get_instruction(use_id);
      if (use->is_phi()) {
        auto maybe_operand_id =
          use->remove_phi_operand(succ_bb->id, builder.context);
        if (maybe_operand_id.has_value()) {
          use->add_phi_operand(
            maybe_operand_id.value(), curr_bb->id, builder.context
          );
        }
      }
    }

    // Remove succ_bb
    succ_bb->remove(builder.context);
  }
}
}  // namespace ir
}  // namespace syc