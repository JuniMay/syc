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

  std::vector<BasicBlockID> pred_list;
  std::vector<BasicBlockID> succ_list;

  BasicBlock(BasicBlockID id, std::string parent_function_name);

  void add_pred(BasicBlockID pred_id);
  void add_succ(BasicBlockID succ_id);

  void remove_pred(BasicBlockID pred_id);
  void remove_succ(BasicBlockID succ_id);

  std::string get_label() const { return ".bb_" + std::to_string(id); }

  void prepend_instruction(InstructionPtr instruction);
  void append_instruction(InstructionPtr instruction);

  void insert_next(BasicBlockPtr basic_block);
  void insert_prev(BasicBlockPtr basic_block);

  std::string to_string(Context& context);
};

BasicBlockPtr
create_basic_block(BasicBlockID id, std::string parent_function_name);

BasicBlockPtr create_dummy_basic_block();

}  // namespace backend

}  // namespace syc

#endif