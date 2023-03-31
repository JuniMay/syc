#include "backend/function.h"

namespace syc {
namespace backend {

void Function::append_basic_block(BasicBlockID basic_block_id) {
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

}  // namespace backend
}  // namespace syc