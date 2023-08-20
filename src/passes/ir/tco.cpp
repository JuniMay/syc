#include "passes/ir/tco.h"

namespace syc {

namespace ir {

void tco(
    Builder& builder
) {
  for(auto &it : builder.context.function_table)
  {
    auto function = it.second;
    if(function->is_declare)
      continue;
    tail_call_elim_func(function, builder);
  }
}

void tail_call_elim_func(
    FunctionPtr function,
    Builder& builder
) {
  // Traverse instructions and find call insts
  std::vector<InstructionPtr> tail_calls;
  for (auto now_bb = function->head_basic_block->next;
      now_bb != function->tail_basic_block;
      now_bb = now_bb->next)
  {
    for (auto now_inst = now_bb->head_instruction->next;
        now_inst != now_bb->tail_instruction;
        now_inst = now_inst->next)
    {
      if (auto call_inst = std::get_if<instruction::Call>(&now_inst->kind))
      {
        auto call_target = call_inst->function_name;
        if (call_target == function->name && is_inst_tail_call(now_inst, function, builder))
        {
          tail_calls.push_back(now_inst);
        }
      }
    }
  }

  if (tail_calls.empty())
    return;

  // Create a new entry basic block
  auto old_entry_block = function->head_basic_block->next;
  auto entry_block = builder.fetch_basic_block();
  builder.set_curr_basic_block(entry_block);
  auto entry_inst = builder.fetch_br_instruction(old_entry_block->id);
  entry_block->append_instruction(entry_inst);
  function->prepend_basic_block(entry_block);
  entry_block->add_succ(old_entry_block->id);
  old_entry_block->add_pred(entry_block->id);

  // Add jump instructions
  for (auto tail_call : tail_calls)
  {
    builder.set_curr_basic_block(builder.context.basic_block_table[tail_call->parent_block_id]);
    auto new_jump_inst = builder.fetch_br_instruction(old_entry_block->id);
    // tail_call->insert_next(new_jump_inst);
    tail_call->insert_prev(new_jump_inst);
    builder.context.basic_block_table[tail_call->parent_block_id]->add_succ(old_entry_block->id);
    old_entry_block->add_pred(tail_call->parent_block_id);
    // tail_call->remove(builder.context);
  }

  // Create new parameters for the function & Insert phi instructions
  auto old_parameter_id_list = std::vector<OperandID>(function->parameter_id_list);
  function->parameter_id_list.clear();
  builder.set_curr_basic_block(old_entry_block);
  for (auto old_parameter_id : old_parameter_id_list)
  {
    auto old_parameter = builder.context.get_operand(old_parameter_id);
    auto new_name = std::get<operand::Parameter>(old_parameter->kind).name + "_real_call";
    auto new_parameter_id = builder.fetch_parameter_operand(old_parameter->get_type(), new_name);
    function->parameter_id_list.push_back(new_parameter_id);

    std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list;
    incoming_list.push_back(std::make_tuple(new_parameter_id, entry_block->id));
    auto arg_index = std::distance(old_parameter_id_list.begin(), 
      std::find(old_parameter_id_list.begin(), old_parameter_id_list.end(), old_parameter_id));
    for (auto tail_call : tail_calls)
    {
      auto call_inst = std::get<instruction::Call>(tail_call->kind);
      auto call_parameter_id = call_inst.arg_id_list[arg_index];
      incoming_list.push_back(std::make_tuple(call_parameter_id, tail_call->parent_block_id));
    }
    auto phi_inst = builder.fetch_phi_instruction(old_parameter_id, incoming_list);
    old_entry_block->prepend_instruction(phi_inst);
    old_parameter->kind = operand::Arbitrary();
    old_parameter->set_def(phi_inst->id);
  }
  

  // Delete tail calls
  for (auto tail_call : tail_calls)
  {
    auto tail_call_prev = tail_call->prev;
    auto parent_block = builder.context.basic_block_table[tail_call->parent_block_id];
    while(tail_call_prev.lock()->next != parent_block->tail_instruction)
      tail_call_prev.lock()->next->remove(builder.context);
    // tail_call->remove(builder.context);
  }
}

bool is_inst_tail_call(
    InstructionPtr inst,
    FunctionPtr function,
    Builder& builder
) {
  // Refactor: judge tail call based 
  return std::visit(overloaded {
    [&](instruction::Call& call_inst) {
      return is_inst_tail_call(inst->next, function, builder) &&
        function->name == call_inst.function_name &&
        function->parameter_id_list.size() < 30;
    },
    [&](instruction::Br& br_inst) {
      auto next_block = builder.context.basic_block_table[br_inst.block_id];
      return is_inst_tail_call(next_block->head_instruction->next, function, builder);
    },
    [&](instruction::CondBr& cond_br_inst) {
      auto then_block = builder.context.basic_block_table[cond_br_inst.then_block_id];
      auto else_block = builder.context.basic_block_table[cond_br_inst.else_block_id];
      return is_inst_tail_call(then_block->head_instruction->next, function, builder) &&
           is_inst_tail_call(else_block->head_instruction->next, function, builder);
    },
    [&](instruction::Ret& ret_inst) {
      return true;
    },
    [&](instruction::Phi& phi_inst) {
      return is_inst_tail_call(inst->next, function, builder);
    },
    [&](auto& inst) {
      return false;
    }
  }, inst->kind);
}


}   // namespace ir

}  // namespace syc