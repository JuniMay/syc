#ifndef SYC_BACKEND_FUNCTION_H_
#define SYC_BACKEND_FUNCTION_H_

#include "common.h"

namespace syc {
namespace backend {

struct Function {
  std::string name;

  std::size_t stack_frame_size;
  std::size_t align_frame_size;

  std::set<int> saved_general_register_list;
  std::set<int> saved_float_register_list;

  BasicBlockPtr head_basic_block;
  BasicBlockPtr tail_basic_block;

  Function(std::string name);

  void append_basic_block(BasicBlockPtr basic_block);

  std::string to_string(Context& context);

  void insert_saved_general_register(int i);

  void insert_saved_float_register(int i);
};

}  // namespace backend
}  // namespace syc

#endif