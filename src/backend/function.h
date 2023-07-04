#ifndef SYC_BACKEND_FUNCTION_H_
#define SYC_BACKEND_FUNCTION_H_

#include "common.h"

namespace syc {
namespace backend {

struct Function {
  std::string name;

  std::size_t stack_frame_size;

  BasicBlockPtr head_basic_block;
  BasicBlockPtr tail_basic_block;

  Function(std::string name);

  void append_basic_block(BasicBlockPtr basic_block);

  std::string to_string(Context& context);
};

}  // namespace backend
}  // namespace syc

#endif