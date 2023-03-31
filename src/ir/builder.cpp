#include "ir/builder.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {
namespace ir {

void Builder::switch_function(std::string function_name) {
  curr_function = context.get_function(function_name);
}

void Builder::switch_basic_block(BasicBlockID basic_block_id) {
  if (
    std::find(
      curr_function->basic_block_list.begin(),
      curr_function->basic_block_list.end(),
      basic_block_id) == curr_function->basic_block_list.end()) {
    throw std::runtime_error("Basic block not found in function.");
  }
  curr_basic_block = context.get_basic_block(basic_block_id);
}

OperandID Builder::make_operand(TypePtr type, OperandKind kind) {
  auto id = context.get_next_operand_id();
  auto operand = std::make_shared<Operand>(Operand{
    id,
    type,
    kind,
    std::nullopt,
    {},
  });
  context.register_operand(operand);
  return id;
}

OperandID
Builder::make_immediate_operand(TypePtr type, std::variant<int, float> value) {
  return make_operand(type, operand::Immediate{value});
}

OperandID Builder::make_parameter_operand(TypePtr type, std::string name) {
  return make_operand(type, operand::Parameter{name});
}

OperandID Builder::make_global_operand(
  TypePtr type,
  std::string name,
  bool is_constant,
  bool is_zero_initialized,
  std::vector<OperandID> initializer
) {
  return make_operand(
    type, operand::Global{name, is_constant, is_zero_initialized, initializer}
  );
}

OperandID Builder::make_arbitrary_operand(TypePtr type) {
  return make_operand(type, operand::Arbitrary{});
}

InstructionPtr Builder::make_binary_instruction(
  instruction::BinaryOp op,
  OperandID dst_id,
  OperandID lhs_id,
  OperandID rhs_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Binary{op, dst_id, lhs_id, rhs_id});

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[lhs_id]->add_use(id);
  context.operand_table[rhs_id]->add_use(id);

  return instruction;
}

InstructionPtr Builder::make_icmp_instruction(
  instruction::ICmpCond cond,
  OperandID dst_id,
  OperandID lhs_id,
  OperandID rhs_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::ICmp{cond, dst_id, lhs_id, rhs_id});

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[lhs_id]->add_use(id);
  context.operand_table[rhs_id]->add_use(id);

  return instruction;
}

InstructionPtr Builder::make_fcmp_instruction(
  instruction::FCmpCond cond,
  OperandID dst_id,
  OperandID lhs_id,
  OperandID rhs_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::FCmp{cond, dst_id, lhs_id, rhs_id});

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[lhs_id]->add_use(id);
  context.operand_table[rhs_id]->add_use(id);

  return instruction;
}

InstructionPtr Builder::make_cast_instruction(
  instruction::CastOp op,
  OperandID dst_id,
  OperandID src_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Cast{op, dst_id, src_id});

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[src_id]->add_use(id);

  return instruction;
}

InstructionPtr Builder::make_ret_instruction(
  std::optional<OperandID> maybe_value_id
) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Ret{maybe_value_id});

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  if (maybe_value_id.has_value()) {
    context.operand_table[maybe_value_id.value()]->add_use(id);
  }

  return instruction;
}

InstructionPtr Builder::make_condbr_instruction(
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

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[cond_id]->add_use(id);
  context.basic_block_table[then_block_id]->add_use(id);
  context.basic_block_table[else_block_id]->add_use(id);

  return instruction;
}

InstructionPtr Builder::make_br_instruction(BasicBlockID block_id) {
  auto id = context.get_next_instruction_id();

  auto kind = InstructionKind(instruction::Br{block_id});

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.basic_block_table[block_id]->add_use(id);

  return instruction;
}

InstructionPtr Builder::make_phi_instruction(
  OperandID dst_id,
  std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Phi{dst_id, incoming_list});

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  // NOT SURE if the phi instruction should be added to the use list.
  context.operand_table[dst_id]->set_def(id);
  for (auto [value_id, block_id] : incoming_list) {
    context.operand_table[value_id]->add_use(id);
    context.basic_block_table[block_id]->add_use(id);
  }

  return instruction;
}

InstructionPtr Builder::make_alloca_instruction(
  OperandID dst_id,
  TypePtr allocaed_type,
  std::optional<OperandID> maybe_size_id,
  std::optional<OperandID> maybe_align_id,
  std::optional<OperandID> maybe_addrspace_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Alloca{
    dst_id,
    allocaed_type,
    maybe_size_id,
    maybe_align_id,
    maybe_addrspace_id,
  });

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  if (maybe_size_id.has_value()) {
    context.operand_table[maybe_size_id.value()]->add_use(id);
  }
  if (maybe_align_id.has_value()) {
    context.operand_table[maybe_align_id.value()]->add_use(id);
  }
  if (maybe_addrspace_id.has_value()) {
    context.operand_table[maybe_addrspace_id.value()]->use_id_list.push_back(id
    );
  }

  return instruction;
}

InstructionPtr Builder::make_load_instruction(
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

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[ptr_id]->add_use(id);
  if (maybe_align_id.has_value()) {
    context.operand_table[maybe_align_id.value()]->add_use(id);
  }

  return instruction;
}

InstructionPtr Builder::make_store_instruction(
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

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[value_id]->add_use(id);
  context.operand_table[ptr_id]->add_use(id);
  if (maybe_align_id.has_value()) {
    context.operand_table[maybe_align_id.value()]->add_use(id);
  }

  return instruction;
}

InstructionPtr Builder::make_call_instruction(
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

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  if (maybe_dst_id.has_value()) {
    context.operand_table[maybe_dst_id.value()]->set_def(id);
  }
  for (auto arg_id : args_id) {
    context.operand_table[arg_id]->add_use(id);
  }
  context.function_table[function_name]->caller_id_list.push_back(id);

  return instruction;
}

InstructionPtr Builder::make_getelementptr_instruction(
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

  auto instruction = make_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[dst_id]->set_def(id);
  context.operand_table[ptr_id]->add_use(id);
  for (auto index_id : indices_id) {
    context.operand_table[index_id]->add_use(id);
  }

  return instruction;
}

void Builder::add_instruction(InstructionPtr instruction) {
  curr_basic_block->add_instruction(instruction);
}

BasicBlockID Builder::add_basic_block() {
  auto id = context.get_next_basic_block_id();
  auto basic_block = std::make_shared<BasicBlock>(id, curr_function->name);
  context.register_basic_block(basic_block);
  curr_function->add_basic_block(id);
  curr_basic_block = basic_block;
  return id;
}

void Builder::add_function(
  std::string function_name,
  std::vector<OperandID> parameter_id_list,
  TypePtr return_type
) {
  auto function = std::make_shared<Function>(Function{
    function_name, return_type, parameter_id_list, {}});
  context.register_function(function);
  curr_function = function;
}

}  // namespace ir
}  // namespace syc