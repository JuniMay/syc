#include "ir/function.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {
namespace ir {

Function::Function(
  std::string name,
  TypePtr return_type,
  std::vector<OperandID> parameter_id_list,
  bool is_declare
)
  : name(name),
    return_type(return_type),
    parameter_id_list(parameter_id_list),
    is_declare(is_declare) {
  this->head_basic_block = create_dummy_basic_block();
  this->tail_basic_block = create_dummy_basic_block();

  this->maybe_return_operand_id = std::nullopt;
  this->maybe_return_block = std::nullopt;

  this->head_basic_block->insert_next(this->tail_basic_block);
}

void Function::append_basic_block(BasicBlockPtr basic_block) {
  if (this->maybe_return_block.has_value()) {
    this->maybe_return_block.value()->insert_prev(basic_block);
  } else {
    this->tail_basic_block->insert_prev(basic_block);
  }
}

std::string Function::to_string(Context& context) {
  if (this->is_declare) {
    std::string result =
      "declare " + type::to_string(return_type) + " @" + name + "(";
    for (auto operand_id : parameter_id_list) {
      auto operand = context.get_operand(operand_id);
      result += type::to_string(operand->get_type()) + ", ";
    }
    if (parameter_id_list.size() > 0) {
      result.pop_back();
      result.pop_back();
    }
    result += ")\n";
    return result;
  }

  std::string result =
    "define " + type::to_string(return_type) + " @" + name + "(";

  for (auto operand_id : parameter_id_list) {
    auto operand = context.get_operand(operand_id);

    result +=
      type::to_string(operand->get_type()) + " " + operand->to_string() + ", ";
  }

  if (parameter_id_list.size() > 0) {
    result.pop_back();
    result.pop_back();
  }

  result += ") {\n";

  auto curr_basic_block = this->head_basic_block->next;

  while (curr_basic_block != this->tail_basic_block) {
    result += curr_basic_block->to_string(context);
    curr_basic_block = curr_basic_block->next;
  }

  result += "}\n";

  return result;
}

void Function::add_terminators(Builder& builder) {
  auto curr_basic_block = this->head_basic_block->next;
  while (curr_basic_block->next != this->tail_basic_block) {
    if (curr_basic_block->has_terminator()) {
      curr_basic_block = curr_basic_block->next;
      continue;
    } else {
      curr_basic_block->append_instruction(
        builder.fetch_br_instruction(curr_basic_block->next->id)
      );
      curr_basic_block = curr_basic_block->next;
    }
  }
}

void Function::remove_caller(InstructionID caller_id) {
  this->caller_id_list.erase(
    std::remove(
      this->caller_id_list.begin(), this->caller_id_list.end(), caller_id
    ),
    this->caller_id_list.end()
  );
}

void Function::remove_unused_basic_blocks(Context& context) {
  // skip the entry block
  auto curr_basic_block = this->head_basic_block->next->next;
  while (curr_basic_block != this->tail_basic_block) {
    auto next_basic_block = curr_basic_block->next;
    if (!curr_basic_block->has_use()) {
      curr_basic_block->remove(context);
    }
    curr_basic_block = next_basic_block;
  }
}

}  // namespace ir
}  // namespace syc