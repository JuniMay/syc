#ifndef SYC_BACKEND_FUNCTION_H_
#define SYC_BACKEND_FUNCTION_H_

#include "backend__register.h"
#include "common.h"

namespace syc {
namespace backend {

struct Function {
  std::string name;

  std::size_t stack_frame_size;
  std::size_t align_frame_size;

  /// Saved registers used in the function
  std::set<Register> saved_register_set;

  BasicBlockPtr head_basic_block;
  BasicBlockPtr tail_basic_block;

  Function(std::string name);

  void append_basic_block(BasicBlockPtr basic_block);

  std::string to_string(Context& context);

  void add_saved_register(Register reg);
};

}  // namespace backend
}  // namespace syc

#endif