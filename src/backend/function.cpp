#include "backend/function.h"
#include "backend/basic_block.h"

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

}  // namespace backend
}  // namespace syc