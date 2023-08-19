#include "passes/ir/straighten.h"
#include "ir/operand.h"

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
  // Policy: fuse blocks that has one succ and the succ has one pred
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block &&
         curr_bb->next != function->tail_basic_block) {
    auto next_bb = curr_bb->next;

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

  // Policy:
  //     A
  //    / \
  //   B   C
  //    \ /
  //     D
  // => if B has only one branch, then
  //     A
  //     |\
  //     | C
  //     |/
  //     D
  // B is removed

  curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    if (curr_bb->succ_list.size() != 2) {
      curr_bb = curr_bb->next;
      continue;
    }

    auto succ0_bb = builder.context.get_basic_block(curr_bb->succ_list[0]);
    auto succ1_bb = builder.context.get_basic_block(curr_bb->succ_list[1]);

    if (succ0_bb->succ_list.size() != 1 || succ1_bb->succ_list.size() != 1) {
      curr_bb = curr_bb->next;
      continue;
    }

    if (succ0_bb->succ_list[0] != succ1_bb->succ_list[0]) {
      curr_bb = curr_bb->next;
      continue;
    }

    auto merge_bb = builder.context.get_basic_block(succ0_bb->succ_list[0]);

    // Decide block to-be-removed (only one branch instruction)
    // succ0 is to-be-removed and succ1 is to-be-kept
    // otherwise just swap
    bool is_succ0_only_branch = true;
    bool is_succ1_only_branch = true;

    for (auto instr = succ0_bb->head_instruction->next;
         instr != succ0_bb->tail_instruction; instr = instr->next) {
      if (!instr->is_br()) {
        is_succ0_only_branch = false;
        break;
      }
    }

    for (auto instr = succ1_bb->head_instruction->next;
         instr != succ1_bb->tail_instruction; instr = instr->next) {
      if (!instr->is_br()) {
        is_succ1_only_branch = false;
        break;
      }
    }

    if (!is_succ0_only_branch && !is_succ1_only_branch) {
      curr_bb = curr_bb->next;
      continue;
    }

    if (!is_succ0_only_branch && is_succ1_only_branch) {
      std::swap(succ0_bb, succ1_bb);
    }

    if (succ0_bb->pred_list.size() != 1) {
      curr_bb = curr_bb->next;
      continue;
    }

    // Modify curr_bb's branch
    curr_bb->remove_succ(succ0_bb->id);
    merge_bb->remove_pred(succ0_bb->id);
    auto condbr_instr = curr_bb->tail_instruction->prev.lock();
    auto& condbr = condbr_instr->as_ref<instruction::CondBr>().value().get();
    if (condbr.then_block_id == succ0_bb->id) {
      condbr.then_block_id = merge_bb->id;
    } else {
      condbr.else_block_id = merge_bb->id;
    }
    curr_bb->add_succ(merge_bb->id);
    merge_bb->add_pred(curr_bb->id);

    // Modify phi instructions
    auto use_id_list_copy = succ0_bb->use_id_list;
    for (auto use_id : use_id_list_copy) {
      auto use = builder.context.get_instruction(use_id);
      if (use->is_phi()) {
        auto maybe_operand_id =
          use->remove_phi_operand(succ0_bb->id, builder.context);
        if (maybe_operand_id.has_value()) {
          use->add_phi_operand(
            maybe_operand_id.value(), curr_bb->id, builder.context
          );
        }
      }
    }

    // Remove succ0_bb
    succ0_bb->remove(builder.context);

    curr_bb = curr_bb->next;
  }

}
}  // namespace ir
}  // namespace syc