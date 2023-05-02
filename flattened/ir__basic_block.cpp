#include "ir__basic_block.h"
#include "ir__context.h"
#include "ir__instruction.h"

namespace syc {
namespace ir {

BasicBlockPtr
create_basic_block(BasicBlockID id, std::string parent_function_name) {
  return std::make_shared<BasicBlock>(id, parent_function_name);
}

BasicBlockPtr create_dummy_basic_block() {
  return std::make_shared<BasicBlock>(
    std::numeric_limits<BasicBlockID>::max(), ""
  );
}

BasicBlock::BasicBlock(BasicBlockID id, std::string parent_function_name)
  : id(id), parent_function_name(parent_function_name), next(nullptr) {
  this->head_instruction = create_dummy_instruction();
  this->tail_instruction = create_dummy_instruction();

  this->head_instruction->insert_next(this->tail_instruction);
}

void BasicBlock::insert_next(BasicBlockPtr basic_block) {
  basic_block->next = this->next;
  basic_block->prev = this->shared_from_this();

  if (this->next) {
    this->next->prev = basic_block;
  }

  this->next = basic_block;
}

void BasicBlock::insert_prev(BasicBlockPtr basic_block) {
  basic_block->next = this->shared_from_this();
  basic_block->prev = this->prev;

  if (auto prev = this->prev.lock()) {
    prev->next = basic_block;
  }

  this->prev = basic_block;
}

void BasicBlock::remove() {
  if (auto prev = this->prev.lock()) {
    prev->next = this->next;
  }

  if (this->next) {
    this->next->prev = this->prev;
  }
}

std::string BasicBlock::to_string(Context& context) {
  std::string label = this->get_label();
  std::string result = label + ":\n";

  auto curr_instruction = this->head_instruction->next;

  while (curr_instruction != this->tail_instruction) {
    result += "  " + curr_instruction->to_string(context) + "\n";
    curr_instruction = curr_instruction->next;
  };

  return result;
}

void BasicBlock::append_instruction(InstructionPtr instruction) {
  this->tail_instruction->insert_prev(instruction);
}

void BasicBlock::add_use(InstructionID use_id) {
  this->use_id_list.push_back(use_id);
}

}  // namespace ir
}  // namespace syc