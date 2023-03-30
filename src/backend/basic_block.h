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

  std::string get_label() const { return ".bb_" + std::to_string(id); }
};

}  // namespace backend

}  // namespace syc

#endif