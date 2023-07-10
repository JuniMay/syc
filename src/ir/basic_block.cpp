#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/instruction.h"

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

void BasicBlock::remove(Context& context) {
  auto curr_instruction = this->head_instruction->next;

  while (curr_instruction != this->tail_instruction) {
    auto next_instruction = curr_instruction->next;
    curr_instruction->remove(context);
    curr_instruction = next_instruction;
  }

  if (auto prev = this->prev.lock()) {
    prev->next = this->next;
  }

  if (this->next) {
    this->next->prev = this->prev;
  }
}

std::string BasicBlock::to_string(Context& context) {
  std::string label = this->get_label();
  std::string result = label + ": ";

  result += "; use count: " + std::to_string(this->use_id_list.size());
  result += "; succ: ";
  for (auto succ_id : this->succ_list) {
    result += std::to_string(succ_id) + ", ";
  }
  result += "; pred: ";
  for (auto pred_id : this->pred_list) {
    result += std::to_string(pred_id) + ", ";
  }
  result += "\n";

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

void BasicBlock::prepend_instruction(InstructionPtr instruction) {
  this->head_instruction->insert_next(instruction);
}

void BasicBlock::add_use(InstructionID use_id) {
  this->use_id_list.push_back(use_id);
}

void BasicBlock::remove_use(InstructionID use_id) {
  this->use_id_list.erase(
    std::remove(this->use_id_list.begin(), this->use_id_list.end(), use_id),
    this->use_id_list.end()
  );
}

void BasicBlock::add_pred(BasicBlockID pred_id) {
  this->pred_list.push_back(pred_id);
}
void BasicBlock::add_succ(BasicBlockID succ_id) {
  this->succ_list.push_back(succ_id);
}
void BasicBlock::remove_pred(BasicBlockID pred_id) {
  this->pred_list.erase(
    std::remove(this->pred_list.begin(), this->pred_list.end(), pred_id),
    this->pred_list.end()
  );
}
void BasicBlock::remove_succ(BasicBlockID succ_id) {
  this->succ_list.erase(
    std::remove(this->succ_list.begin(), this->succ_list.end(), succ_id),
    this->succ_list.end()
  );
}

bool BasicBlock::has_terminator() const {
  return this->tail_instruction->prev.lock()->is_terminator();
}

bool BasicBlock::has_use() const {
  return !this->use_id_list.empty();
}

std::vector<BasicBlockID> BasicBlock::get_succ() const {
  std::vector<BasicBlockID> result;
  if (this->head_instruction->next == this->tail_instruction)
    return result;
  auto tail_instruction_ptr = this->tail_instruction->prev.lock();
  if (tail_instruction_ptr) {
    std::visit(
      overloaded{
        [&](const ir::instruction::Br& kind) {
          result = std::vector<BasicBlockID>{kind.block_id};
        },
        [&](const ir::instruction::CondBr& kind) {
          result =
            std::vector<BasicBlockID>{kind.then_block_id, kind.else_block_id};
        },
        [](const auto&) {
          // Do nothing for other instruction kinds
        }},
      tail_instruction_ptr->kind
    );
  }
  return result;
}

}  // namespace ir
}  // namespace syc