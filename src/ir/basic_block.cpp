#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/instruction.h"

namespace syc {
namespace ir {

std::string BasicBlock::to_string(Context& context) {
  std::string label = this->get_label();
  std::string result = label + ":\n";

  for (auto instruction_id : instruction_list) {
    auto instruction = context.get_instruction(instruction_id);
    result += "  " + instruction->to_string(context) + "\n";
  }

  return result;
}

void BasicBlock::insert_instruction_after(
  InstructionID instruction_id,
  InstructionID after_instruction_id
) {
  auto it = std::find(
    instruction_list.begin(), instruction_list.end(), after_instruction_id
  );
  if (it == instruction_list.end()) {
    throw std::runtime_error("Instruction not found");
  }
  ++it;
  instruction_list.insert(it, instruction_id);
}

void BasicBlock::insert_instruction_before(
  InstructionID instruction_id,
  InstructionID before_instruction_id
) {
  auto it = std::find(
    instruction_list.begin(), instruction_list.end(), before_instruction_id
  );
  if (it == instruction_list.end()) {
    throw std::runtime_error("Instruction not found");
  }
  instruction_list.insert(it, instruction_id);
}

void BasicBlock::add_use(InstructionID use_id) {
  use_id_list.push_back(use_id);
}

}  // namespace ir
}  // namespace syc