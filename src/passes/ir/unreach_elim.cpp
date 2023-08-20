#include "passes/ir/unreach_elim.h"

namespace syc {

namespace ir {

void unreach_elim(Builder& builder) {
  for (auto& it : builder.context.function_table) {
    auto function = it.second;
    if (function->is_declare)
      continue;
    elim_const_branch(function, builder);
    remove_unreach_block(function, builder);
  }
}

void elim_const_branch(FunctionPtr function, Builder& builder) {
  for (auto now_bb =
         function->head_basic_block->next;  // skip dummy basic block
       now_bb != function->tail_basic_block; now_bb = now_bb->next) {
    InstructionPtr tail_inst = now_bb->tail_instruction->prev.lock();

    if (auto cond_br_inst = std::get_if<instruction::CondBr>(&tail_inst->kind)) {
      auto cond_op_id = cond_br_inst->cond_id;
      auto cond_op = builder.context.get_operand(cond_op_id);
      auto kind = cond_op->kind;

      if (auto const_op = std::get_if<operand::ConstantPtr>(&cond_op->kind)) {
        auto target_block_id = (*const_op)->get_bool_value()
                                 ? cond_br_inst->then_block_id
                                 : cond_br_inst->else_block_id;
        builder.set_curr_basic_block(
          builder.context.basic_block_table[target_block_id]
        );
        auto br_inst = builder.fetch_br_instruction(target_block_id);
        tail_inst->remove(builder.context);
        builder.set_curr_basic_block(now_bb);
        builder.append_instruction(br_inst);
      }
    }
  }
}

void remove_unreach_block(FunctionPtr function, Builder& builder) {
  auto reached_blocks = get_reachable_blocks(function, builder);
  for (auto now_bb =
         function->head_basic_block->next;  // skip dummy basic block
       now_bb != function->tail_basic_block; now_bb = now_bb->next) {
    if (reached_blocks.count(now_bb->id) == 0) {
      now_bb->remove(builder.context);
    }
  }

  // remove inexistent incoming blocks in all phi insts
  for (auto now_bb =
         function->head_basic_block->next;  // skip dummy basic block
       now_bb != function->tail_basic_block; now_bb = now_bb->next) {
    for (auto now_inst = now_bb->head_instruction->next;
         now_inst != now_bb->tail_instruction; now_inst = now_inst->next) {
      if (auto phi_inst = std::get_if<instruction::Phi>(&now_inst->kind)) {
        auto incoming_list_copy = phi_inst->incoming_list;
        for (auto& [_, block_id] : incoming_list_copy) {
          if (reached_blocks.count(block_id) == 0) {
            now_inst->remove_phi_operand(block_id, builder.context);
          }
          if (std::find(now_bb->pred_list.begin(), now_bb->pred_list.end(), 
                block_id) == now_bb->pred_list.end()) {
            now_inst->remove_phi_operand(block_id, builder.context);
          }
        }
      }
    }
  }
}

std::set<BasicBlockID>
get_reachable_blocks(FunctionPtr function, Builder& builder) {
  // std::cout << function->name << std::endl;
  auto basic_block_table = builder.context.basic_block_table;
  std::set<BasicBlockID> reached_blocks;
  std::stack<BasicBlockID> block_stack;
  BasicBlockID visiting_block;

  block_stack.push(function->head_basic_block->next->id);
  while (!block_stack.empty()) {
    visiting_block = block_stack.top();
    block_stack.pop();
    if (reached_blocks.count(visiting_block) != 0)
      continue;
    reached_blocks.insert(visiting_block);
    // std::cout << visiting_block << std::endl;
    auto succ_list = basic_block_table[visiting_block]->get_succ();
    for (auto& it : succ_list) {
      if (reached_blocks.count(it) == 0)
        block_stack.push(it);
    }
  }
  return reached_blocks;
}

}  // namespace ir

}  // namespace syc