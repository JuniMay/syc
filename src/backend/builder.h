#ifndef SYC_BACKEND_BUILDER_H_
#define SYC_BACKEND_BUILDER_H_

#include "backend/context.h"
#include "common.h"

namespace syc {
namespace backend {

struct Builder {
  Context context;

  BasicBlockPtr curr_basic_block;
};

}  // namespace backend
}  // namespace syc

#endif