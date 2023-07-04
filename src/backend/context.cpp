#include "backend/context.h"
#include "backend/basic_block.h"
#include "backend/function.h"
#include "backend/global.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "common.h"
#include "ir/operand.h"

namespace syc {
namespace backend {

void Context::register_operand(OperandPtr operand) {
  if (operand_table.find(operand->id) != operand_table.end()) {
    throw std::runtime_error("Operand ID already exists.");
  }
  operand_table[operand->id] = operand;

  // If the operand is global, add it to the global list.
  if (std::holds_alternative<Global>(operand->kind) && operand->modifier == Modifier::None) {
    global_list.push_back(operand->id);
  }
}

void Context::register_instruction(InstructionPtr instruction) {
  if (instruction_table.find(instruction->id) != instruction_table.end()) {
    throw std::runtime_error("Instruction ID already exists.");
  }
  instruction_table[instruction->id] = instruction;
}

void Context::register_basic_block(BasicBlockPtr basic_block) {
  if (basic_block_table.find(basic_block->id) != basic_block_table.end()) {
    throw std::runtime_error("BasicBlock ID already exists.");
  }
  basic_block_table[basic_block->id] = basic_block;
}

void Context::register_function(FunctionPtr function) {
  if (function_table.find(function->name) != function_table.end()) {
    throw std::runtime_error("Function name already exists.");
  }
  function_table[function->name] = function;
}

OperandPtr Context::get_operand(OperandID id) {
  return operand_table.at(id);
}

InstructionPtr Context::get_instruction(InstructionID id) {
  return instruction_table.at(id);
}

BasicBlockPtr Context::get_basic_block(BasicBlockID id) {
  return basic_block_table.at(id);
}

FunctionPtr Context::get_function(std::string name) {
  return function_table.at(name);
}

std::string Context::to_string() {
  // generate asm code
  std::string result = "\t.option pic\n";
  result += "\t.text\n";

  for (auto& func : this->function_table) {
    result += func.second->to_string(*this);
  }

  for (auto& operand_id : this->global_list) {
    auto& global = std::get<Global>(this->operand_table.at(operand_id)->kind);
    result += global.to_string() + "\n";
  }

  return result;
}

}  // namespace backend
}  // namespace syc