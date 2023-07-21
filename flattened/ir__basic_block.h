#ifndef SYC_IR_BASIC_BLOCK_H_
#define SYC_IR_BASIC_BLOCK_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__context.h"

namespace syc {
namespace ir {

struct BasicBlock : std::enable_shared_from_this<BasicBlock> {
  /// ID of the basic block in the context
  BasicBlockID id;

  /// Name of the parent function
  std::string parent_function_name;

  /// Head instruction
  /// The head instruction is a dummy instruction to index the initial
  /// instruction.
  InstructionPtr head_instruction;
  /// Tail instruction
  /// The tail instruction is a dummy instruction to index the final
  /// instruction.
  InstructionPtr tail_instruction;

  /// List of id of instructions that use this block
  std::vector<InstructionID> use_id_list;

  std::vector<BasicBlockID> pred_list;
  std::vector<BasicBlockID> succ_list;

  /// The next basic block.
  BasicBlockPtr next;
  /// The previous basic block.
  BasicBlockPrevPtr prev;

  /// Constructor
  BasicBlock(BasicBlockID id, std::string parent_function_name);

  /// Append instruction to the end of the basic block.
  void append_instruction(InstructionPtr instruction);

  /// Prepend instruction to the beginning of the basic block.
  void prepend_instruction(InstructionPtr instruction);

  /// Add a instruction id that use this block to `use_id_list`
  void add_use(InstructionID use_id);

  void remove_use(InstructionID use_id);

  // Split the block into two blocks.
  // This will not add branch at the end of the block.
  // This will not check if the instruction is within the block.
  void split(InstructionPtr instruction, Builder& builder);

  void add_pred(BasicBlockID pred_id);
  void add_succ(BasicBlockID succ_id);
  void remove_pred(BasicBlockID pred_id);
  void remove_succ(BasicBlockID succ_id);

  /// Insert the basic block after the current basic block.
  void insert_next(BasicBlockPtr basic_block);
  /// Insert the basic block before the current basic block.
  void insert_prev(BasicBlockPtr basic_block);
  /// Remove the basic block from the linked list.
  void remove(Context& context);

  /// Get the label name of the basic block.
  /// Note that the returned string cannot be directly used as an IR operand.
  std::string get_label() const { return "bb_" + std::to_string(id); }

  /// Convert the basic block and its instructions to a string in IR form.
  std::string to_string(Context& context);

  /// If the last instruction is a terminator.
  bool has_terminator() const;

  /// If the block is used.
  bool has_use() const;

  std::vector<BasicBlockID> get_succ() const;
};

/// Create a basic block.
BasicBlockPtr
create_basic_block(BasicBlockID id, std::string parent_function_name);

/// Create a dummy basic block.
BasicBlockPtr create_dummy_basic_block();

}  // namespace ir
}  // namespace syc

#endif