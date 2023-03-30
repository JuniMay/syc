#ifndef SYC_BACKEND_BASIC_BLOCK_H_
#define SYC_BACKEND_BASIC_BLOCK_H_

#include "common.h"

namespace syc {
namespace backend {

/// Machine basic block.
struct BasicBlock {
  BasicBlockID id;

  std::string parent_function_name;

  std::list<InstructionID> instruction_list;
};

}  // namespace backend

}  // namespace syc

#endif