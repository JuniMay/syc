#ifndef SYC_IR_INSTRUCTION_H_
#define SYC_IR_INSTRUCTION_H_

#include "common.h"

namespace syc {
namespace ir {
namespace instruction {

/// Opcode enum of the binary instruction.
enum class BinaryOp {
  /// Add two integers.
  Add,
  /// Subtract two integers.
  Sub,
  /// Multiply two integers.
  Mul,
  /// Divide two signed integers.
  SDiv,
  /// Divide two floating point numbers.
  FAdd,
  /// Subtract two floating point numbers.
  FSub,
  /// Multiply two floating point numbers.
  FMul,
  /// Divide two floating point numbers.
  FDiv,
};

/// Binary instructions.
struct Binary {
  /// Opcode.
  BinaryOp op;
  /// Destination operand ID.
  OperandID dst_id;
  /// Left hand side operand ID.
  OperandID lhs_id;
  /// Right hand side operand ID.
  OperandID rhs_id;
};

/// Opcode enum of the icmp instruction.
enum class ICmpCond {
  /// Equal
  Eq,
  /// Not equal
  Ne,
  /// Signed less than
  Slt,
};

struct ICmp {
  /// Condition.
  ICmpCond cond;
  /// Destination operand ID.
  OperandID dst_id;
  /// Left hand side operand ID.
  OperandID lhs_id;
  /// Right hand side operand ID.
  OperandID rhs_id;
};

/// Opcode enum of the fcmp instruction.
enum class FCmpCond {
  /// (ordered and) Equal
  Oeq,
  /// (ordered and) Less than
  Olt,
  /// (ordered and) Less than or equal
  Ole,
};

struct FCmp {
  /// Condition.
  FCmpCond cond;
  /// Destination operand ID.
  OperandID dst_id;
  /// Left hand side operand ID.
  OperandID lhs_id;
  /// Right hand side operand ID.
  OperandID rhs_id;
};

/// Opcode enum of the cast instruction.
enum class CastOp {
  /// Zero extend.
  ZExt,
  /// Bitwise cast.
  BitCast,
  /// Floating point to signed integer.
  FPToSI,
  /// Signed integer to floating point.
  SIToFP,
};

/// This extends the operand to the dst type.
struct Cast {
  /// Opcode.
  CastOp op;
  /// Destination operand ID.
  OperandID dst_id;
  /// Source operand ID.
  OperandID src_id;
};

/// Return instruction.
///
/// If the return type and return value are nullopt, it is a void return.
struct Ret {
  /// Return value operand ID.
  std::optional<OperandID> maybe_value_id;
};

/// Conditional branch instruction.
struct CondBr {
  /// Condition operand ID.
  ///
  /// The condition operand must be a boolean/i1 type.
  OperandID cond_id;
  /// Block ID of the true branch.
  BasicBlockID then_block_id;
  /// Block ID of the false branch.
  BasicBlockID else_block_id;
};

/// Unconditional branch instruction.
struct Br {
  /// Label ID of the branch.
  BasicBlockID block_id;
};

/// Phi instruction.
struct Phi {
  /// Destination operand ID.
  OperandID dst_id;
  /// List of incoming operands.
  ///
  /// This is a list of (operand ID, block ID) pairs.
  std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list;
};

/// Allocate memory on the stack.
struct Alloca {
  /// Destination operand ID.
  OperandID dst_id;
  /// Type of the allocated memory.
  TypePtr allocated_type;
  /// Size of the allocated memory.
  std::optional<OperandID> maybe_size_id;
  /// Alignment size of the allocated memory.
  std::optional<OperandID> maybe_align_id;
  /// Address space of the allocated memory.
  std::optional<OperandID> maybe_addrspace_id;
};

/// Load value from the given address/pointer.
struct Load {
  /// Destination operand ID.
  OperandID dst_id;
  /// Pointer operand ID.
  OperandID ptr_id;
  /// Alignment size of the loaded value.
  std::optional<OperandID> maybe_align_id;
};

/// Store value to the given address.
struct Store {
  /// Value operand ID.
  OperandID value_id;
  /// Pointer operand ID.
  OperandID ptr_id;
  /// Alignment size of the stored value.
  std::optional<OperandID> maybe_align_id;
};

/// Call instruction.
struct Call {
  /// Destination operand ID.
  std::optional<OperandID> maybe_dst_id;
  /// Function operand ID.
  std::string function_name;
  /// List of argument operand IDs.
  std::vector<OperandID> arg_ids;
};

/// GetElementPtr instruction.
struct GetElementPtr {
  /// Destination operand ID.
  OperandID dst_id;
  /// Basis type of the calculation.
  TypePtr basis_type;
  /// Pointer operand ID.
  OperandID ptr_id;
  /// List of indices.
  std::vector<OperandID> index_ids;
};

}  // namespace instruction

struct Instruction {
  InstructionID id;
  InstructionKind kind;

  std::string to_string(Context& context);
};

}  // namespace ir
}  // namespace syc

#endif