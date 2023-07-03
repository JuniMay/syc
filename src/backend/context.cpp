#include "backend/context.h"
#include "backend/function.h"
#include "backend/global.h"
#include "backend/operand.h"
#include "common.h"
#include "ir/operand.h"

namespace syc {
namespace backend {

OperandPtr Context::get_operand(OperandID id) {
  return operand_table.at(id);
}

InstructionPtr Context::get_instruction(InstructionID id) {
  return instruction_table.at(id);
}

BasicBlockPtr Context::get_basic_block(BasicBlockID id) {
  return basic_block_table.at(id);
}

std::string Context::to_string() {
  // generate asm code
  std::string result = "\t.option pic\n";
  result += "\t.text\n";

  for (auto& func : this->function_table) {
    result += func.second->to_string();
  }

  for (auto& operand_id : this->global_list) {
    auto& global = std::get<Global>(this->operand_table.at(operand_id)->kind);
    result += global.to_string() + "\n";
  }

  return result;
}

}  // namespace backend
}  // namespace syc