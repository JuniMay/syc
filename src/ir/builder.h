#ifndef SYC_IR_BUILDER_H_
#define SYC_IR_BUILDER_H_

#include "common.h"
#include "ir/context.h"
#include "ir/type.h"

namespace syc {
namespace ir {

struct Builder {
  /// Context of the intermediate representation.
  Context context;

  /// Current function
  FunctionPtr curr_function;
  /// Current block
  BasicBlockPtr curr_basic_block;

  /// Stack to record the condition basic block of while statement.
  std::stack<BasicBlockPtr> while_cond_basic_block_stack;

  /// Stack to record the tail basic block of while statement.
  std::stack<BasicBlockPtr> while_tail_basic_block_stack;

  Builder() = default;

  TypePtr fetch_i32_type() { return std::make_shared<Type>(type::Integer{32}); }

  TypePtr fetch_i1_type() { return std::make_shared<Type>(type::Integer{1}); }

  TypePtr fetch_float_type() { return std::make_shared<Type>(type::Float{}); }

  TypePtr fetch_void_type() { return std::make_shared<Type>(type::Void{}); }

  TypePtr fetch_pointer_type(TypePtr value_type) {
    return std::make_shared<Type>(type::Pointer{value_type});
  }

  TypePtr fetch_array_type(size_t length, TypePtr value_type) {
    return std::make_shared<Type>(type::Array{length, value_type});
  }

  /// Make an operand and register it to the context.
  /// If the operand is a global item, it will be added to the global list.
  /// This will return the ID of the operand.
  OperandID fetch_operand(TypePtr type, OperandKind kind);

  operand::ConstantPtr fetch_constant(TypePtr type, operand::ConstantKind kind);

  OperandID fetch_constant_operand(TypePtr type, operand::ConstantKind kind);

  OperandID fetch_parameter_operand(TypePtr type, std::string name);

  OperandID fetch_global_operand(
    TypePtr type,
    std::string name,
    bool is_constant_value,
    OperandID init
  );

  OperandID fetch_arbitrary_operand(TypePtr type);

  /// Make a binary instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_binary_instruction(
    instruction::BinaryOp op,
    OperandID dst_id,
    OperandID lhs_id,
    OperandID rhs_id
  );

  /// Make an icmp instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_icmp_instruction(
    instruction::ICmpCond cond,
    OperandID dst_id,
    OperandID lhs_id,
    OperandID rhs_id
  );

  /// Make a fcmp instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_fcmp_instruction(
    instruction::FCmpCond cond,
    OperandID dst_id,
    OperandID lhs_id,
    OperandID rhs_id
  );

  /// Make a cast instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_cast_instruction(
    instruction::CastOp op,
    OperandID dst_id,
    OperandID src_id
  );

  /// Make a sext instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_ret_instruction(
    std::optional<OperandID> maybe_value_id = std::nullopt
  );

  /// Make a conditional branch instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_condbr_instruction(
    OperandID cond_id,
    BasicBlockID then_block_id,
    BasicBlockID else_block_id
  );

  /// Make an unconditional branch instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_br_instruction(BasicBlockID block_id);

  /// Make a phi instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_phi_instruction(
    OperandID dst_id,
    std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list
  );

  /// Make a alloca instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_alloca_instruction(
    OperandID dst_id,
    TypePtr allocated_type,
    std::optional<OperandID> maybe_size_id,
    std::optional<OperandID> maybe_align_id,
    std::optional<OperandID> maybe_addrspace_id
  );

  /// Make a load instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_load_instruction(
    OperandID dst_id,
    OperandID ptr_id,
    std::optional<OperandID> maybe_align
  );

  /// Make a store instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_store_instruction(
    OperandID value_id,
    OperandID ptr_id,
    std::optional<OperandID> maybe_align
  );

  /// Make a call instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_call_instruction(
    std::optional<OperandID> maybe_dst_id,
    std::string function_name,
    std::vector<OperandID> arg_id_list
  );

  /// Make a getelementptr instruction and register it to the context.
  /// Return the pointer to the instruction.
  InstructionPtr fetch_getelementptr_instruction(
    OperandID dst_id,
    TypePtr basis_type,
    OperandID ptr_id,
    std::vector<OperandID> index_id_list
  );

  /// Append an instruction to the current block.
  void append_instruction(InstructionPtr instruction);

  /// Prepend an instruction to the current block.
  void prepend_instruction_to_curr_basic_block(InstructionPtr instruction);

  /// Prepend an instruction to the current function.
  void prepend_instruction_to_curr_function(InstructionPtr instruction);

  /// Create a new block for the current function.
  BasicBlockPtr fetch_basic_block();

  /// Append a block to the current function.
  void append_basic_block(BasicBlockPtr basic_block);

  void set_curr_basic_block(BasicBlockPtr basic_block);

  void switch_function(std::string function_name);

  /// Add a function with given name, operands and return type to the context.
  /// All the operands must be parameters.
  void add_function(
    std::string function_name,
    std::vector<OperandID> parameter_id_list,
    TypePtr return_type,
    bool is_declare = false
  );
};

}  // namespace ir

}  // namespace syc

#endif