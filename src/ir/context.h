#ifndef SYC_IR_CONTEXT_H_
#define SYC_IR_CONTEXT_H_

#include "common.h"

namespace syc {
namespace ir {

struct Context {
  std::map<OperandID, OperandPtr> operand_table;
  std::map<InstructionID, InstructionPtr> instruction_table;
  std::map<BasicBlockID, std::shared_ptr<BasicBlock>> basic_block_table;

  std::map<std::string, std::shared_ptr<Function>> function_table;
  std::vector<OperandID> global_list;

  OperandID next_operand_id = 0;
  InstructionID next_instruction_id = 0;
  BasicBlockID next_basic_block_id = 0;

  Context() = default;

  OperandID get_next_operand_id() { return next_operand_id++; }
  InstructionID get_next_instruction_id() { return next_instruction_id++; }
  BasicBlockID get_next_basic_block_id() { return next_basic_block_id++; }

  void register_operand(OperandPtr operand);

  void register_basic_block(std::shared_ptr<BasicBlock> basic_block);

  void register_instruction(InstructionPtr instruction);

  void register_function(std::shared_ptr<Function> function);

  OperandPtr get_operand(OperandID id);

  InstructionPtr get_instruction(InstructionID id);

  BasicBlockPtr get_basic_block(BasicBlockID id);

  FunctionPtr get_function(std::string function_name);

  std::string to_string();
};

}  // namespace ir
}  // namespace syc

#endif