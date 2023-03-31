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
  std::vector<OperandID> parameter_id_list
)
  : name(name), return_type(return_type), parameter_id_list(parameter_id_list) {
  this->head_basic_block = create_dummy_basic_block();
  this->tail_basic_block = create_dummy_basic_block();

  this->head_basic_block->insert_next(this->tail_basic_block);
}

void Function::append_basic_block(BasicBlockPtr basic_block) {
  this->tail_basic_block->insert_prev(basic_block);
}

std::string Function::to_string(Context& context) {
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

}  // namespace ir
}  // namespace syc