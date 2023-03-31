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

  Builder() = default;

  void switch_function(std::string function_name);

  void switch_basic_block(BasicBlockID basic_block_id);

  TypePtr make_i32_type() { return std::make_shared<Type>(type::Integer{32}); }

  TypePtr make_i1_type() { return std::make_shared<Type>(type::Integer{1}); }

  TypePtr make_float_type() { return std::make_shared<Type>(type::Float{}); }

  TypePtr make_void_type() { return std::make_shared<Type>(type::Void{}); }

  TypePtr make_pointer_type(TypePtr value_type) {
    return std::make_shared<Type>(type::Pointer{value_type});
  }

  TypePtr make_array_type(std::optional<size_t> length, TypePtr value_type) {
    return std::make_shared<Type>(type::Array{length, value_type});
  }

  /// Make an operand and register it to the context.
  /// If the operand is a global item, it will be added to the global list.
  /// This will return the ID of the operand.
  OperandID make_operand(TypePtr type, OperandKind kind);

  OperandID
  make_immediate_operand(TypePtr type, std::variant<int, float> value);

  OperandID make_parameter_operand(TypePtr type, std::string name);

  OperandID make_global_operand(
    TypePtr type,
    std::string name,
    bool is_constant,
    bool is_zero_initialized,
    std::vector<OperandID> initializer
  );

  OperandID make_arbitrary_operand(TypePtr type);

  /// Make a binary instruction
  InstructionPtr make_binary_instruction(
    instruction::BinaryOp op,
    OperandID dst_id,
    OperandID lhs_id,
    OperandID rhs_id
  );

  /// Make an icmp instruction
  InstructionPtr make_icmp_instruction(
    instruction::ICmpCond cond,
    OperandID dst_id,
    OperandID lhs_id,
    OperandID rhs_id
  );

  /// Make a fcmp instruction
  InstructionPtr make_fcmp_instruction(
    instruction::FCmpCond cond,
    OperandID dst_id,
    OperandID lhs_id,
    OperandID rhs_id
  );

  /// Make a cast instruction
  InstructionPtr make_cast_instruction(
    instruction::CastOp op,
    OperandID dst_id,
    OperandID src_id
  );

  /// Make a sext instruction
  InstructionPtr make_ret_instruction(
    std::optional<OperandID> maybe_value_id = std::nullopt
  );

  /// Make a conditional branch instruction
  InstructionPtr make_condbr_instruction(
    OperandID cond_id,
    BasicBlockID then_block_id,
    BasicBlockID else_block_id
  );

  /// Make an unconditional branch instruction
  InstructionPtr make_br_instruction(BasicBlockID block_id);

  /// Make a phi instruction
  InstructionPtr make_phi_instruction(
    OperandID dst_id,
    std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list
  );

  /// Make a alloca instruction
  InstructionPtr make_alloca_instruction(
    OperandID dst_id,
    TypePtr allocated_type,
    std::optional<OperandID> maybe_size_id,
    std::optional<OperandID> maybe_align_id,
    std::optional<OperandID> maybe_addrspace_id
  );

  /// Make a load instruction
  InstructionPtr make_load_instruction(
    OperandID dst_id,
    OperandID ptr_id,
    std::optional<OperandID> maybe_align
  );

  /// Make a store instruction
  InstructionPtr make_store_instruction(
    OperandID value_id,
    OperandID ptr_id,
    std::optional<OperandID> maybe_align
  );

  /// Make a call instruction
  InstructionPtr make_call_instruction(
    std::optional<OperandID> maybe_dst_id,
    std::string function_name,
    std::vector<OperandID> arg_id_list
  );

  /// Make a getelementptr instruction
  InstructionPtr make_getelementptr_instruction(
    OperandID dst_id,
    TypePtr basis_type,
    OperandID ptr_id,
    std::vector<OperandID> index_id_list
  );

  /// Add an instruction to the current block.
  void add_instruction(InstructionPtr instruction);

  /// Create a new block for the current function and set the current block to
  /// the newly created block.
  BasicBlockID add_basic_block();

  /// Add a function with given name, operands and return type to the context.
  /// All the operands must be parameters.
  void add_function(
    std::string function_name,
    std::vector<OperandID> parameter_id_list,
    TypePtr return_type
  );

  std::string to_string() const;
};

}  // namespace ir

}  // namespace syc

#endif