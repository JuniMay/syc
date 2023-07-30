#include "backend__builder.h"

namespace syc {
namespace backend {

OperandID Builder::fetch_operand(OperandKind kind, Modifier modifier) {
  auto id = context.get_next_operand_id();
  auto operand = std::make_shared<Operand>(id, kind, modifier);
  context.register_operand(operand);
  return id;
}

OperandID Builder::fetch_immediate(
  std::variant<int32_t, int64_t, uint32_t, uint64_t> value
) {
  auto kind = Immediate{value};
  return fetch_operand(kind, Modifier::None);
}

OperandID Builder::fetch_global(
  std::string name,
  std::variant<std::vector<uint32_t>, uint64_t> value
) {
  auto kind = Global{value, name};
  return fetch_operand(kind, Modifier::None);
}

OperandID Builder::fetch_virtual_register(VirtualRegisterKind kind) {
  auto vreg_id = context.get_next_virtual_register_id();
  auto vreg = VirtualRegister{vreg_id, kind};
  return fetch_operand(vreg, Modifier::None);
}

OperandID Builder::fetch_register(Register reg) {
  return fetch_operand(reg, Modifier::None);
}

OperandID Builder::fetch_local_memory(int offset) {
  auto kind = LocalMemory{offset};
  return fetch_operand(kind, Modifier::None);
}

BasicBlockPtr Builder::fetch_basic_block() {
  auto id = context.get_next_basic_block_id();
  auto basic_block = create_basic_block(id, curr_function->name);

  context.register_basic_block(basic_block);
  return basic_block;
}

InstructionPtr Builder::fetch_load_instruction(
  instruction::Load::Op op,
  OperandID rd_id,
  OperandID rs_id,
  OperandID imm_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Load{op, rd_id, rs_id, imm_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_load_instruction(
  instruction::FloatLoad::Op op,
  OperandID rd_id,
  OperandID rs_id,
  OperandID imm_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::FloatLoad{op, rd_id, rs_id, imm_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs_id);

  return instruction;
}

InstructionPtr Builder::fetch_store_instruction(
  instruction::Store::Op op,
  OperandID rs1_id,
  OperandID rs2_id,
  OperandID imm_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Store{op, rs1_id, rs2_id, imm_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rs1_id]->add_use(id);
  context.operand_table[rs2_id]->add_use(id);

  instruction->add_use(rs1_id);
  instruction->add_use(rs2_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_store_instruction(
  instruction::FloatStore::Op op,
  OperandID rs1_id,
  OperandID rs2_id,
  OperandID imm_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::FloatStore{op, rs1_id, rs2_id, imm_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rs1_id]->add_use(id);
  context.operand_table[rs2_id]->add_use(id);

  instruction->add_use(rs1_id);
  instruction->add_use(rs2_id);

  return instruction;
}

InstructionPtr Builder::fetch_pseudo_load_instruction(
  instruction::PseudoLoad::Op op,
  OperandID rd_id,
  OperandID symbol_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::PseudoLoad{op, rd_id, symbol_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);

  instruction->add_def(rd_id);

  return instruction;
}

InstructionPtr Builder::fetch_pseudo_store_instruction(
  instruction::PseudoStore::Op op,
  OperandID rd_id,
  OperandID symbol_id,
  OperandID rt_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::PseudoStore{op, rd_id, symbol_id, rt_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_use(id);
  // rt is defined by auipc
  context.operand_table[rt_id]->add_def(id);
  context.operand_table[rt_id]->add_use(id);

  instruction->add_use(rd_id);
  instruction->add_def(rt_id);
  instruction->add_use(rt_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_pseudo_load_instruction(
  instruction::FloatPseudoLoad::Op op,
  OperandID rd_id,
  OperandID symbol_id,
  OperandID rt_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::FloatPseudoLoad{op, rd_id, symbol_id, rt_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  // rt is defined by auipc
  context.operand_table[rt_id]->add_def(id);
  context.operand_table[rt_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_def(rt_id);
  instruction->add_use(rt_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_pseudo_store_instruction(
  instruction::FloatPseudoStore::Op op,
  OperandID rd_id,
  OperandID symbol_id,
  OperandID rt_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::FloatPseudoStore{op, rd_id, symbol_id, rt_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_use(id);
  // rt is defined by auipc
  context.operand_table[rt_id]->add_def(id);
  context.operand_table[rt_id]->add_use(id);

  instruction->add_use(rd_id);
  instruction->add_def(rt_id);
  instruction->add_use(rt_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_move_instruction(
  instruction::FloatMove::Fmt dst_fmt,
  instruction::FloatMove::Fmt src_fmt,
  OperandID rd_id,
  OperandID rs_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::FloatMove{dst_fmt, src_fmt, rd_id, rs_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_convert_instruction(
  instruction::FloatConvert::Fmt dst_fmt,
  instruction::FloatConvert::Fmt src_fmt,
  OperandID rd_id,
  OperandID rs_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::FloatConvert{dst_fmt, src_fmt, rd_id, rs_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs_id);

  return instruction;
}

InstructionPtr Builder::fetch_binary_instruction(
  instruction::Binary::Op op,
  OperandID rd_id,
  OperandID rs1_id,
  OperandID rs2_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Binary{op, rd_id, rs1_id, rs2_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs1_id]->add_use(id);
  context.operand_table[rs2_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs1_id);
  instruction->add_use(rs2_id);

  return instruction;
}

InstructionPtr Builder::fetch_binary_imm_instruction(
  instruction::BinaryImm::Op op,
  OperandID rd_id,
  OperandID rs_id,
  OperandID imm_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::BinaryImm{op, rd_id, rs_id, imm_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_binary_instruction(
  instruction::FloatBinary::Op op,
  instruction::FloatBinary::Fmt fmt,
  OperandID rd_id,
  OperandID rs1_id,
  OperandID rs2_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::FloatBinary{op, fmt, rd_id, rs1_id, rs2_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs1_id]->add_use(id);
  context.operand_table[rs2_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs1_id);
  instruction->add_use(rs2_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_mul_add_instruction(
  instruction::FloatMulAdd::Op op,
  instruction::FloatMulAdd::Fmt fmt,
  OperandID rd_id,
  OperandID rs1_id,
  OperandID rs2_id,
  OperandID rs3_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::FloatMulAdd{
    op, fmt, rd_id, rs1_id, rs2_id, rs3_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs1_id]->add_use(id);
  context.operand_table[rs2_id]->add_use(id);
  context.operand_table[rs3_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs1_id);
  instruction->add_use(rs2_id);
  instruction->add_use(rs3_id);

  return instruction;
}

InstructionPtr Builder::fetch_float_unary_instruction(
  instruction::FloatUnary::Op op,
  instruction::FloatUnary::Fmt fmt,
  OperandID rd_id,
  OperandID rs_id
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::FloatUnary{op, fmt, rd_id, rs_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);
  context.operand_table[rs_id]->add_use(id);

  instruction->add_def(rd_id);
  instruction->add_use(rs_id);

  return instruction;
}

InstructionPtr
Builder::fetch_lui_instruction(OperandID rd_id, OperandID imm_id) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Lui{rd_id, imm_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);

  instruction->add_def(rd_id);

  return instruction;
}

InstructionPtr
Builder::fetch_li_instruction(OperandID rd_id, OperandID imm_id) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Li{rd_id, imm_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);

  instruction->add_def(rd_id);

  return instruction;
}

InstructionPtr Builder::fetch_call_instruction(std::string function_name) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Call{function_name});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  return instruction;
}

InstructionPtr Builder::fetch_branch_instruction(
  instruction::Branch::Op op,
  OperandID rs1_id,
  OperandID rs2_id,
  BasicBlockID block_id
) {
  auto id = context.get_next_instruction_id();
  auto kind =
    InstructionKind(instruction::Branch{op, rs1_id, rs2_id, block_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rs1_id]->add_use(id);
  context.operand_table[rs2_id]->add_use(id);

  instruction->add_use(rs1_id);
  instruction->add_use(rs2_id);

  return instruction;
}

InstructionPtr Builder::fetch_j_instruction(BasicBlockID block_id) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::J{block_id});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  return instruction;
}

InstructionPtr Builder::fetch_ret_instruction() {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Ret{});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  return instruction;
}

InstructionPtr Builder::fetch_phi_instruction(
  OperandID rd_id,
  std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list
) {
  auto id = context.get_next_instruction_id();
  auto kind = InstructionKind(instruction::Phi{rd_id, incoming_list});
  auto instruction = create_instruction(id, kind, curr_basic_block->id);

  context.register_instruction(instruction);

  context.operand_table[rd_id]->add_def(id);

  instruction->add_def(rd_id);

  for (auto [operand_id, basic_block_id] : incoming_list) {
    context.operand_table[operand_id]->add_use(id);
    instruction->add_use(operand_id);
  }

  return instruction;
}

void Builder::prepend_instruction(InstructionPtr instruction) {
  curr_basic_block->prepend_instruction(instruction);
  instruction->parent_block_id = curr_basic_block->id;
}

void Builder::append_instruction(InstructionPtr instruction) {
  curr_basic_block->append_instruction(instruction);
  auto maybe_basic_block_id = instruction->get_basic_block_id_if_branch();
  if (maybe_basic_block_id.has_value()) {
    curr_basic_block->add_succ(maybe_basic_block_id.value());
    context.basic_block_table[maybe_basic_block_id.value()]->add_pred(
      curr_basic_block->id
    );
  }
}

void Builder::set_curr_basic_block(BasicBlockPtr basic_block) {
  curr_basic_block = basic_block;
}

void Builder::switch_function(std::string function_name) {
  curr_function = context.get_function(function_name);
}

void Builder::add_function(std::string name) {
  auto function = std::make_shared<Function>(name);
  context.register_function(function);
}

}  // namespace backend
}  // namespace syc