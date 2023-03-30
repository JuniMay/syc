#ifndef SYC_IR_OPERAND_H_
#define SYC_IR_OPERAND_H_

#include "common.h"

namespace syc {
namespace ir {

namespace operand {

/// Global variable/constant
struct Global {
  /// Name of the global variable.
  std::string name;
  /// If the global variable is constant.
  bool is_constant;
  /// If the variable is initialized by zero.
  bool is_zero_initialized;
  /// Initial value/initializer of the global variable.
  /// If the variable is an array, this is a list of initial values.
  std::vector<OperandID> initializer;
};

/// Immediate
struct Immediate {
  /// Value of the immediate.
  std::variant<int, float> value;
};

/// Parameter
struct Parameter {
  /// Name of the parameter.
  std::string name;
};

/// Arbitrary operand/variable.
struct Arbitrary {};

using OperandPtr = std::shared_ptr<Operand>;

}  // namespace operand

/// Operand
struct Operand {
  /// ID of the operand.
  OperandID id;
  /// Type of the operand.
  TypePtr type;
  /// Operand kind.
  OperandKind kind;
  /// ID of the instruction that defines this operand.
  std::optional<InstructionID> def_id;
  /// IDs of the instructions that use this operand.
  std::vector<InstructionID> use_id_list;

  /// Get the stringified representation of the operand (in llvm ir).
  std::string to_string() const;

  /// Get the type of the operand.
  /// Note that global variables/constants have a pointer of the type.
  /// If the type is directly accessed, it will not be wrapped into a pointer.
  TypePtr get_type();

  void set_def(InstructionID def_id);

  void add_use(InstructionID use_id);
};

}  // namespace ir
}  // namespace syc

#endif