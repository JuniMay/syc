#ifndef SYC_BACKEND_BUILDER_H_
#define SYC_BACKEND_BUILDER_H_

#include "backend__basic_block.h"
#include "backend__context.h"
#include "backend__function.h"
#include "backend__global.h"
#include "backend__immediate.h"
#include "backend__instruction.h"
#include "backend__operand.h"
#include "backend__register.h"
#include "common.h"

namespace syc {
namespace backend {

struct Builder {
  /// Context of the assembly
  Context context;
  /// Current function
  FunctionPtr curr_function;
  /// Current basic block
  BasicBlockPtr curr_basic_block;

  OperandID fetch_operand(OperandKind kind, Modifier modifier);

  OperandID fetch_immediate(
    std::variant<int32_t, int64_t, uint32_t, uint64_t> value
  );

  OperandID fetch_global(
    std::string name,
    std::variant<std::vector<uint32_t>, uint64_t> value
  );

  OperandID fetch_virtual_register(VirtualRegisterKind kind);

  OperandID fetch_register(Register reg);

  OperandID fetch_local_memory(int offset);

  BasicBlockPtr fetch_basic_block();

  InstructionPtr fetch_load_instruction(
    instruction::Load::Op op,
    OperandID rd_id,
    OperandID rs_id,
    OperandID imm_id
  );

  InstructionPtr fetch_float_load_instruction(
    instruction::FloatLoad::Op op,
    OperandID rd_id,
    OperandID rs_id,
    OperandID imm_id
  );

  InstructionPtr fetch_store_instruction(
    instruction::Store::Op op,
    OperandID rs1_id,
    OperandID rs2_id,
    OperandID imm_id
  );

  InstructionPtr fetch_float_store_instruction(
    instruction::FloatStore::Op op,
    OperandID rs1_id,
    OperandID rs2_id,
    OperandID imm_id
  );

  InstructionPtr fetch_pseudo_load_instruction(
    instruction::PseudoLoad::Op op,
    OperandID rd_id,
    OperandID symbol_id
  );

  InstructionPtr fetch_pseudo_store_instruction(
    instruction::PseudoStore::Op op,
    OperandID rd_id,
    OperandID symbol_id,
    OperandID rt_id
  );

  InstructionPtr fetch_float_pseudo_load_instruction(
    instruction::FloatPseudoLoad::Op op,
    OperandID rd_id,
    OperandID symbol_id,
    OperandID rt_id
  );

  InstructionPtr fetch_float_pseudo_store_instruction(
    instruction::FloatPseudoStore::Op op,
    OperandID rd_id,
    OperandID symbol_id,
    OperandID rt_id
  );

  InstructionPtr fetch_float_move_instruction(
    instruction::FloatMove::Fmt dst_fmt,
    instruction::FloatMove::Fmt src_fmt,
    OperandID rd_id,
    OperandID rs_id
  );

  InstructionPtr fetch_float_convert_instruction(
    instruction::FloatConvert::Fmt dst_fmt,
    instruction::FloatConvert::Fmt src_fmt,
    OperandID rd_id,
    OperandID rs_id
  );

  InstructionPtr fetch_binary_instruction(
    instruction::Binary::Op op,
    OperandID rd_id,
    OperandID rs1_id,
    OperandID rs2_id
  );

  InstructionPtr fetch_binary_imm_instruction(
    instruction::BinaryImm::Op op,
    OperandID rd_id,
    OperandID rs_id,
    OperandID imm_id
  );

  InstructionPtr fetch_float_binary_instruction(
    instruction::FloatBinary::Op op,
    instruction::FloatBinary::Fmt fmt,
    OperandID rd_id,
    OperandID rs1_id,
    OperandID rs2_id
  );

  InstructionPtr fetch_float_mul_add_instruction(
    instruction::FloatMulAdd::Op op,
    instruction::FloatMulAdd::Fmt fmt,
    OperandID rd_id,
    OperandID rs1_id,
    OperandID rs2_id,
    OperandID rs3_id
  );

  InstructionPtr fetch_float_unary_instruction(
    instruction::FloatUnary::Op op,
    instruction::FloatUnary::Fmt fmt,
    OperandID rd_id,
    OperandID rs_id
  );

  InstructionPtr fetch_ret_instruction();

  InstructionPtr fetch_lui_instruction(OperandID rd_id, OperandID imm_id);

  InstructionPtr fetch_li_instruction(OperandID rd_id, OperandID imm_id);

  InstructionPtr fetch_j_instruction(BasicBlockID block_id);

  InstructionPtr fetch_phi_instruction(
    OperandID rd_id,
    std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list
  );

  InstructionPtr fetch_call_instruction(std::string function_name);

  InstructionPtr fetch_branch_instruction(
    instruction::Branch::Op op,
    OperandID rs1_id,
    OperandID rs2_id,
    BasicBlockID block_id
  );

  // Prepend instruction to current basic block
  void prepend_instruction(InstructionPtr instruction);

  void append_instruction(InstructionPtr instruction);

  void set_curr_basic_block(BasicBlockPtr basic_block);

  void switch_function(std::string function_name);

  void add_function(std::string name);

  Builder() = default;
};

}  // namespace backend
}  // namespace syc

#endif