#ifndef SYC_BACKEND_BASIC_BLOCK_H_
#define SYC_BACKEND_BASIC_BLOCK_H_

#include "common.h"

namespace syc {
namespace backend {

/// Machine basic block.
struct BasicBlock : std::enable_shared_from_this<BasicBlock> {
  BasicBlockID id;

  std::string parent_function_name;

  InstructionPtr head_instruction;
  InstructionPtr tail_instruction;

  BasicBlockPtr next;
  BasicBlockPrevPtr prev;

  BasicBlock(BasicBlockID id, std::string parent_function_name);

  std::string get_label() const { return ".bb_" + std::to_string(id); }

  void append_instruction(InstructionPtr instruction);

  void insert_next(BasicBlockPtr basic_block);
  void insert_prev(BasicBlockPtr basic_block);
};

BasicBlockPtr
create_basic_block(BasicBlockID id, std::string parent_function_name);

BasicBlockPtr create_dummy_basic_block();

}  // namespace backend

}  // namespace syc

#endif