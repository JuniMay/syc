#include "ir/function.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {
namespace ir {

void Function::add_basic_block(BasicBlockID basic_block_id) {
  basic_block_list.push_back(basic_block_id);
}

void Function::remove_basic_block(BasicBlockID basic_block_id) {
  basic_block_list.remove(basic_block_id);
}

void Function::insert_basic_block_after(
  BasicBlockID basic_block_id,
  BasicBlockID insert_after_id
) {
  auto it = std::find(
    basic_block_list.begin(), basic_block_list.end(), insert_after_id
  );
  if (it == basic_block_list.end()) {
    throw std::runtime_error("insert_after_id not found");
  }
  ++it;
  basic_block_list.insert(it, basic_block_id);
}

void Function::insert_basic_block_before(
  BasicBlockID basic_block_id,
  BasicBlockID insert_before_id
) {
  auto it = std::find(
    basic_block_list.begin(), basic_block_list.end(), insert_before_id
  );
  if (it == basic_block_list.end()) {
    throw std::runtime_error("insert_before_id not found");
  }
  basic_block_list.insert(it, basic_block_id);
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

  for (auto basic_block_id : basic_block_list) {
    auto basic_block = context.get_basic_block(basic_block_id);
    result += basic_block->to_string(context);
  }

  result += "}\n";

  return result;
}

}  // namespace ir
}  // namespace syc