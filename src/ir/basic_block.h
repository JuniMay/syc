#ifndef SYC_IR_BASIC_BLOCK_H_
#define SYC_IR_BASIC_BLOCK_H_

#include "common.h"
#include "ir/context.h"

namespace syc {
namespace ir {

struct BasicBlock {
  BasicBlockID id;
  std::string parent_function_name;
  std::list<InstructionID> instruction_list;

  std::vector<InstructionID> use_id_list;

  void add_instruction(InstructionID instruction_id) {
    instruction_list.push_back(instruction_id);
  }

  void insert_instruction_after(
    InstructionID instruction_id,
    InstructionID after_instruction_id
  );

  void insert_instruction_before(
    InstructionID instruction_id,
    InstructionID before_instruction_id
  );

  std::string get_label() const { return "bb_" + std::to_string(id); }

  std::string to_string(Context& context);

  void add_use(InstructionID use_id);
};

}  // namespace ir
}  // namespace syc

#endif