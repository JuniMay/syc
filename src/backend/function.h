#ifndef SYC_BACKEND_FUNCTION_H_
#define SYC_BACKEND_FUNCTION_H_

#include "common.h"

namespace syc {
namespace backend {

struct Function {
  std::string name;

  std::size_t stack_frame_size;

  std::list<BasicBlockID> basic_block_list;

  void append_basic_block(BasicBlockID basic_block_id);
  void remove_basic_block(BasicBlockID basic_block_id);

  void insert_basic_block_after(
    BasicBlockID basic_block_id,
    BasicBlockID insert_after_id
  );

  void insert_basic_block_before(
    BasicBlockID basic_block_id,
    BasicBlockID insert_before_id
  );
};

}  // namespace backend
}  // namespace syc

#endif