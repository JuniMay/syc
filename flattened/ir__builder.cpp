#include "ir__builder.h"
#include "ir__basic_block.h"
#include "ir__context.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"
#include "ir__type.h"

namespace syc {
namespace ir {

OperandID Builder::fetch_operand(TypePtr type, OperandKind kind) {
  auto id = context.get_next_operand_id();
  auto operand = std::make_shared<Operand>(id, type, kind);
  context.register_operand(operand);
  return id;
}

OperandID
Builder::fetch_constant_operand(TypePtr type, operand::ConstantKind kind) {
  return fetch_operand(
    type, std::make_shared<operand::Constant>(operand::Constant{kind, type})
  );
}

operand::ConstantPtr
Builder::fetch_constant(TypePtr type, operand::ConstantKind kind) {
  return std::make_shared<operand::Constant>(operand::Constant{kind, type});
}

OperandID Builder::fetch_parameter_operand(TypePtr type, std::string name) {
  return fetch_operand(type, operand::Parameter{name});
}

OperandID Builder::fetch_global_operand(
  TypePtr type,
  std::string name,
  bool is_constant_value,
  OperandID init
) {
  return fetch_operand(type, operand::Global{name, is_constant_value, init});
}

OperandID Builder::fetch_arbitrary_operand(TypePtr type) {
  return fetch_operand(type, operand::Arbitrary{});
}

InstructionPtr Builder::fetch_binary_instruction(
  instruction::BinaryOp op,
  OperandID dst_id,
  OperandID lhs_id,
  OperandID rhs_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Binary{op, dst_id, lhs_id, rhs_id});

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[lhs_id]->add_use(id);
  context.operand_table[rhs_id]->add_use(id);

  instruction->set_def(dst_id);
  instruction->add_use(lhs_id);
  instruction->add_use(rhs_id);

  return instruction;
}

InstructionPtr Builder::fetch_icmp_instruction(
  instruction::ICmpCond cond,
  OperandID dst_id,
  OperandID lhs_id,
  OperandID rhs_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::ICmp{cond, dst_id, lhs_id, rhs_id});

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[lhs_id]->add_use(id);
  context.operand_table[rhs_id]->add_use(id);

  instruction->set_def(dst_id);
  instruction->add_use(lhs_id);
  instruction->add_use(rhs_id);

  return instruction;
}

InstructionPtr Builder::fetch_fcmp_instruction(
  instruction::FCmpCond cond,
  OperandID dst_id,
  OperandID lhs_id,
  OperandID rhs_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::FCmp{cond, dst_id, lhs_id, rhs_id});

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[lhs_id]->add_use(id);
  context.operand_table[rhs_id]->add_use(id);

  instruction->set_def(dst_id);
  instruction->add_use(lhs_id);
  instruction->add_use(rhs_id);

  return instruction;
}

InstructionPtr Builder::fetch_cast_instruction(
  instruction::CastOp op,
  OperandID dst_id,
  OperandID src_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Cast{op, dst_id, src_id});

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[src_id]->add_use(id);

  instruction->set_def(dst_id);
  instruction->add_use(src_id);

  return instruction;
}

InstructionPtr Builder::fetch_ret_instruction(
  std::optional<OperandID> maybe_value_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Ret{maybe_value_id});

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  if (maybe_value_id.has_value()) {
    context.operand_table[maybe_value_id.value()]->add_use(id);
    instruction->add_use(maybe_value_id.value());
  }

  return instruction;
}

InstructionPtr Builder::fetch_condbr_instruction(
  OperandID cond_id,
  BasicBlockID then_block_id,
  BasicBlockID else_block_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::CondBr{
    cond_id,
    then_block_id,
    else_block_id,
  });

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[cond_id]->add_use(id);
  context.basic_block_table[then_block_id]->add_use(id);
  context.basic_block_table[else_block_id]->add_use(id);

  instruction->add_use(cond_id);

  return instruction;
}

InstructionPtr Builder::fetch_br_instruction(BasicBlockID block_id) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Br{block_id});

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.basic_block_table[block_id]->add_use(id);

  return instruction;
}

InstructionPtr Builder::fetch_phi_instruction(
  OperandID dst_id,
  std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Phi{dst_id, incoming_list});

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  instruction->set_def(dst_id);

  // NOT SURE if the phi instruction should be added to the use list.
  context.operand_table[dst_id]->set_def(id);
  for (auto [value_id, block_id] : incoming_list) {
    context.operand_table[value_id]->add_use(id);
    context.basic_block_table[block_id]->add_use(id);
    instruction->add_use(value_id);
  }

  return instruction;
}

InstructionPtr Builder::fetch_alloca_instruction(
  OperandID dst_id,
  TypePtr allocaed_type,
  std::optional<OperandID> maybe_size_id,
  std::optional<OperandID> maybe_align_id,
  std::optional<OperandID> maybe_addrspace_id,
  bool alloca_for_param
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Alloca{
    dst_id,
    allocaed_type,
    maybe_size_id,
    maybe_align_id,
    maybe_addrspace_id,
    alloca_for_param,
  });

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  instruction->set_def(dst_id);

  context.operand_table[dst_id]->set_def(id);
  if (maybe_size_id.has_value()) {
    context.operand_table[maybe_size_id.value()]->add_use(id);
    instruction->add_use(maybe_size_id.value());
  }
  if (maybe_align_id.has_value()) {
    context.operand_table[maybe_align_id.value()]->add_use(id);
    instruction->add_use(maybe_align_id.value());
  }
  if (maybe_addrspace_id.has_value()) {
    context.operand_table[maybe_addrspace_id.value()]->use_id_list.push_back(id
    );
    instruction->add_use(maybe_addrspace_id.value());
  }

  return instruction;
}

InstructionPtr Builder::fetch_load_instruction(
  OperandID dst_id,
  OperandID ptr_id,
  std::optional<OperandID> maybe_align_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Load{
    dst_id,
    ptr_id,
    maybe_align_id,
  });

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[ptr_id]->add_use(id);

  instruction->set_def(dst_id);
  instruction->add_use(ptr_id);

  if (maybe_align_id.has_value()) {
    context.operand_table[maybe_align_id.value()]->add_use(id);
    instruction->add_use(maybe_align_id.value());
  }

  return instruction;
}

InstructionPtr Builder::fetch_store_instruction(
  OperandID value_id,
  OperandID ptr_id,
  std::optional<OperandID> maybe_align_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Store{
    value_id,
    ptr_id,
    maybe_align_id,
  });

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[value_id]->add_use(id);
  context.operand_table[ptr_id]->add_use(id);

  instruction->add_use(value_id);
  instruction->add_use(ptr_id);

  if (maybe_align_id.has_value()) {
    context.operand_table[maybe_align_id.value()]->add_use(id);
    instruction->add_use(maybe_align_id.value());
  }

  return instruction;
}

InstructionPtr Builder::fetch_call_instruction(
  std::optional<OperandID> maybe_dst_id,
  std::string function_name,
  std::vector<OperandID> args_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Call{
    maybe_dst_id,
    function_name,
    args_id,
  });

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  if (maybe_dst_id.has_value()) {
    context.operand_table[maybe_dst_id.value()]->set_def(id);
    instruction->set_def(maybe_dst_id.value());
  }
  for (auto arg_id : args_id) {
    context.operand_table[arg_id]->add_use(id);
    instruction->add_use(arg_id);
  }
  context.function_table[function_name]->caller_id_list.push_back(id);

  return instruction;
}

InstructionPtr Builder::fetch_getelementptr_instruction(
  OperandID dst_id,
  TypePtr basis_type,
  OperandID ptr_id,
  std::vector<OperandID> indices_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::GetElementPtr{
    dst_id,
    basis_type,
    ptr_id,
    indices_id,
  });

  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[ptr_id]->add_use(id);

  instruction->set_def(dst_id);
  instruction->add_use(ptr_id);

  for (auto index_id : indices_id) {
    context.operand_table[index_id]->add_use(id);
    instruction->add_use(index_id);
  }

  return instruction;
}

void Builder::append_instruction(InstructionPtr instruction) {
  if (curr_basic_block->has_terminator()) {
    return;
  }

  curr_basic_block->append_instruction(instruction);

  if (std::holds_alternative<instruction::CondBr>(instruction->kind)) {
    const auto& condbr = std::get<instruction::CondBr>(instruction->kind);

    curr_basic_block->add_succ(condbr.then_block_id);
    curr_basic_block->add_succ(condbr.else_block_id);

    context.basic_block_table[condbr.then_block_id]->add_pred(
      curr_basic_block->id
    );
    context.basic_block_table[condbr.else_block_id]->add_pred(
      curr_basic_block->id
    );
  } else if (std::holds_alternative<instruction::Br>(instruction->kind)) {
    const auto& br = std::get<instruction::Br>(instruction->kind);

    curr_basic_block->add_succ(br.block_id);
    context.basic_block_table[br.block_id]->add_pred(curr_basic_block->id);
  }
}

/// Prepend an instruction to the current block.
void Builder::prepend_instruction_to_curr_basic_block(InstructionPtr instruction
) {
  curr_basic_block->prepend_instruction(instruction);
}

/// Prepend an instruction to the current function.
void Builder::prepend_instruction_to_curr_function(InstructionPtr instruction) {
  curr_function->head_basic_block->next->prepend_instruction(instruction);
}

BasicBlockPtr Builder::fetch_basic_block() {
  auto id = context.get_next_basic_block_id();
  auto basic_block = create_basic_block(id, curr_function->name);

  context.register_basic_block(basic_block);
  return basic_block;
}

void Builder::append_basic_block(BasicBlockPtr basic_block) {
  curr_function->append_basic_block(basic_block);
}

void Builder::set_curr_basic_block(BasicBlockPtr basic_block) {
  curr_basic_block = basic_block;
}

void Builder::switch_function(std::string function_name) {
  curr_function = context.get_function(function_name);
}

void Builder::add_function(
  std::string function_name,
  std::vector<OperandID> parameter_id_list,
  TypePtr return_type,
  bool is_declare
) {
  auto function = std::make_shared<Function>(
    function_name, return_type, parameter_id_list, is_declare
  );
  context.register_function(function);
  curr_function = function;
}

}  // namespace ir
}  // namespace syc