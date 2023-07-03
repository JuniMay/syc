#include "backend/context.h"
#include "ir/operand.h"
#include "common.h"
#include "backend/operand.h"
//#include "backend/global.h"
#include "backend/function.h"

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

std::string Context::to_string()
{
  // generate asm code
  std::string result = "\t.option pic\n";
  result += "\t.text\n";
  for (auto &global : this->global_list)
  {
    // if(auto global = std::get_if<backend::Global>(&operand.second))
    result += "\t.globl " + global.name + "\n";
    result += "\t.data\n";
    result += "\t.align 2\n";
    result += "\t.type " + global.name + ", @object\n";
    result += "\t.size " + global.name + ", " + std::to_string(global.get_size()) + "\n";
    result += global.name + ":\n";
    result += "\t.word " + global.value_string() + "\n";
  }

  result += "\t.text\n";

  for (auto &func : this->function_table)
  {
    result += func.second->to_string();
  }

  return result;
}

}  // namespace backend
}  // namespace syc