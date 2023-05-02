#ifndef SYC_IR_FUNCTION_H_
#define SYC_IR_FUNCTION_H_

#include "common.h"
#include "ir__basic_block.h"

namespace syc {
namespace ir {

struct Function {
  /// Function name
  std::string name;
  /// Return type.
  TypePtr return_type;
  /// List of paramter operand ID.
  std::vector<OperandID> parameter_id_list;
  /// Head basic block of the function.
  /// This block is a dummy block to index the initial basic block.
  BasicBlockPtr head_basic_block;
  /// Tail basic block of the function.
  /// This block is a dummy block to index the final basic block.
  BasicBlockPtr tail_basic_block;
  /// List of id of instructions that use this function
  std::vector<InstructionID> caller_id_list;

  bool is_declare;

  /// Constructor
  Function(
    std::string name,
    TypePtr return_type,
    std::vector<OperandID> parameter_id_list,
    bool is_declare = false
  );

  /// Append basic block to the end of the function.
  void append_basic_block(BasicBlockPtr basic_block);

  /// Convert the function and its basic blocks to a string in IR form.
  std::string to_string(Context& context);
};

}  // namespace ir
}  // namespace syc

#endif