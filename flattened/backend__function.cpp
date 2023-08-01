#include "backend__function.h"
#include "backend__basic_block.h"
#include "backend__context.h"
#include "common.h"

namespace syc {
namespace backend {

Function::Function(std::string name) : name(name), stack_frame_size(0) {
  this->head_basic_block = create_dummy_basic_block();
  this->tail_basic_block = create_dummy_basic_block();
  this->head_basic_block->insert_next(this->tail_basic_block);
}

void Function::append_basic_block(BasicBlockPtr basic_block) {
  this->tail_basic_block->insert_prev(basic_block);
}

std::string Function::to_string(Context& context) {
  std::string result = "\t.globl " + this->name + "\n";
  result += "\t.align 1\n";
  result += "\t.type " + this->name + ", @function\n";
  result += this->name + ":\n";

  auto curr_basic_block = this->head_basic_block->next;

  while (curr_basic_block != this->tail_basic_block) {
    result += curr_basic_block->to_string(context);
    curr_basic_block = curr_basic_block->next;
  }

  return result;
}

void Function::add_saved_register(Register reg) {
  if (!REG_CALLEE_SAVED.count(reg)) {
    return;
  }
  this->saved_register_set.insert(reg);
}

}  // namespace backend
}  // namespace syc