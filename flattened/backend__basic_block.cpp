#include "backend__basic_block.h"
#include "backend__instruction.h"

namespace syc {
namespace backend {

BasicBlock::BasicBlock(BasicBlockID id, std::string parent_function_name)
  : id(id), parent_function_name(parent_function_name), next(nullptr) {
  this->head_instruction = create_dummy_instruction();
  this->tail_instruction = create_dummy_instruction();
  this->head_instruction->insert_next(this->tail_instruction);
}

void BasicBlock::add_pred(BasicBlockID pred_id) {
  this->pred_list.push_back(pred_id);
}

void BasicBlock::add_succ(BasicBlockID succ_id) {
  this->succ_list.push_back(succ_id);
}

void BasicBlock::remove_pred(BasicBlockID pred_id) {
  auto it = std::find(this->pred_list.begin(), this->pred_list.end(), pred_id);
  if (it != this->pred_list.end()) {
    this->pred_list.erase(it);
  }
}
void BasicBlock::remove_succ(BasicBlockID succ_id) {
  auto it = std::find(this->succ_list.begin(), this->succ_list.end(), succ_id);
  if (it != this->succ_list.end()) {
    this->succ_list.erase(it);
  }
}

void BasicBlock::prepend_instruction(InstructionPtr instruction) {
  this->head_instruction->insert_next(instruction);
}

void BasicBlock::append_instruction(InstructionPtr instruction) {
  this->tail_instruction->insert_prev(instruction);
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

std::string BasicBlock::to_string(Context& context) {
  std::string result = this->get_label() + ":\n";

  result += "\t# preds = ";
  for (auto pred_id : this->pred_list) {
    result += std::to_string(pred_id) + ", ";
  }
  result += "; succs = ";
  for (auto succ_id : this->succ_list) {
    result += std::to_string(succ_id) + ", ";
  }
  result += "\n";

  auto curr_instruction = this->head_instruction->next;

  while (curr_instruction != this->tail_instruction) {
    result += "\t" + curr_instruction->to_string(context) + "\n";
    curr_instruction = curr_instruction->next;
  }

  return result;
}

BasicBlockPtr
create_basic_block(BasicBlockID id, std::string parent_function_name) {
  return std::make_shared<BasicBlock>(id, parent_function_name);
}

BasicBlockPtr create_dummy_basic_block() {
  return std::make_shared<BasicBlock>(
    std::numeric_limits<BasicBlockID>::max(), ""
  );
}

}  // namespace backend
}  // namespace syc