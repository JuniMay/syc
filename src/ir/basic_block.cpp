#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/instruction.h"

namespace syc {
namespace ir {

BasicBlock::BasicBlock(BasicBlockID id, std::string parent_function_name)
  : id(id), parent_function_name(parent_function_name) {
  this->head_instruction = make_dummy_instruction();
  this->tail_instruction = make_dummy_instruction();

  this->head_instruction->insert_next(this->tail_instruction);
}

std::string BasicBlock::to_string(Context& context) {
  std::string label = this->get_label();
  std::string result = label + ":\n";

  auto curr_instruction = this->head_instruction->next;

  while (curr_instruction != this->tail_instruction) {
    result += "  " + curr_instruction->to_string(context) + "\n";
    curr_instruction = curr_instruction->next;
  };

  return result;
}

void BasicBlock::add_instruction(InstructionPtr instruction) {
  this->tail_instruction->insert_prev(instruction);
}

void BasicBlock::add_use(InstructionID use_id) {
  this->use_id_list.push_back(use_id);
}

}  // namespace ir
}  // namespace syc