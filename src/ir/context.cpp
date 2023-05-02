#include "ir/context.h"
#include "ir/basic_block.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {
namespace ir {

void Context::register_operand(OperandPtr operand) {
  if (operand_table.find(operand->id) != operand_table.end()) {
    throw std::runtime_error("Operand ID already exists.");
  }
  operand_table[operand->id] = operand;

  // If the operand is global, add it to the global list.
  if (std::holds_alternative<operand::Global>(operand->kind)) {
    global_list.push_back(operand->id);
  }
}

void Context::register_basic_block(std::shared_ptr<BasicBlock> basic_block) {
  if (basic_block_table.find(basic_block->id) != basic_block_table.end()) {
    throw std::runtime_error("BasicBlock ID already exists.");
  }
  basic_block_table[basic_block->id] = basic_block;
}

void Context::register_instruction(InstructionPtr instruction) {
  if (instruction_table.find(instruction->id) != instruction_table.end()) {
    throw std::runtime_error("Instruction ID already exists.");
  }
  instruction_table[instruction->id] = instruction;
}

void Context::register_function(std::shared_ptr<Function> function) {
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

FunctionPtr Context::get_function(std::string function_name) {
  return function_table.at(function_name);
}

std::string Context::to_string() {
  std::string result = "";

  // Global variable/constants.
  for (auto operand_id : global_list) {
    auto global = std::get<operand::Global>(operand_table[operand_id]->kind);

    // In declaration, the type of global variable is the type of its value.
    // So here just get the type instead of using `get_type`.
    // Note that if global variable is used as an operand, it must be used as a
    // pointer/address.
    auto type = operand_table[operand_id]->type;

    result += "@" + global.name + " = ";

    if (global.is_constant) {
      result += "constant ";
    } else {
      result += "global ";
    }

    result += type::to_string(type) + " ";

    if (global.is_zero_initialized) {
      result += "zeroinitializer";
    } else {
      if (std::holds_alternative<type::Array>(*type)) {
        result += "[ ";
        for (auto operand_id : global.initializer) {
          auto operand = operand_table[operand_id];
          auto operand_type_str = type::to_string(operand->get_type());
          auto operand_str = operand->to_string();

          result += operand_type_str + " " + operand_str + ", ";
        }

        if (global.initializer.size() > 0) {
          result.pop_back();
          result.pop_back();
        }

        result += " ]";
      } else {
        result += operand_table[global.initializer.at(0)]->to_string();
      }
    }

    result += "\n";
  }
  result += "\n";

  for (auto& [function_name, function] : function_table) {
    result += function->to_string(*this) + "\n";
  }

  return result;
}

}  // namespace ir
}  // namespace syc