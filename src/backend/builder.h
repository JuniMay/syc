#ifndef SYC_BACKEND_BUILDER_H_
#define SYC_BACKEND_BUILDER_H_

#include "backend/context.h"
#include "common.h"

namespace syc {
namespace backend {

struct Builder {
  Context context;

  FunctionPtr curr_function;
  BasicBlockPtr curr_basic_block;

  Builder() = default;
};

}  // namespace backend
}  // namespace syc

#endif