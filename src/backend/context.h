#ifndef SYC_BACKEND_CONTEXT_H_
#define SYC_BACKEND_CONTEXT_H_

#include "common.h"

namespace syc {
namespace backend {

struct Context {
  std::map<OperandID, OperandPtr> operand_table;
  std::map<InstructionID, InstructionPtr> instruction_table;
  std::map<BasicBlockID, BasicBlockPtr> basic_block_table;

  OperandID next_operand_id;
  InstructionID next_instruction_id;
  BasicBlockID next_basic_block_id;

  OperandID get_next_operand_id() { return next_operand_id++; }

  InstructionID get_next_instruction_id() { return next_instruction_id++; }

  BasicBlockID get_next_basic_block_id() { return next_basic_block_id++; }
};

}  // namespace backend
}  // namespace syc

#endif