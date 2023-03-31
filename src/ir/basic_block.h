#ifndef SYC_IR_BASIC_BLOCK_H_
#define SYC_IR_BASIC_BLOCK_H_

#include "common.h"
#include "ir/context.h"

namespace syc {
namespace ir {

struct BasicBlock {
  BasicBlockID id;
  std::string parent_function_name;
  
  InstructionPtr head_instruction;
  InstructionPtr tail_instruction;

  std::vector<InstructionID> use_id_list;

  BasicBlock(BasicBlockID id, std::string parent_function_name);

  void add_instruction(InstructionPtr instruction);

  std::string get_label() const { return "bb_" + std::to_string(id); }

  std::string to_string(Context& context);

  void add_use(InstructionID use_id);
};

}  // namespace ir
}  // namespace syc

#endif