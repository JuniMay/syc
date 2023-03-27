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

}  // namespace ir
}  // namespace syc