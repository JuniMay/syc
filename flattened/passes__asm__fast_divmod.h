#ifndef SYC_PASSES_ASM_FAST_DIVMOD_H_
#define SYC_PASSES_ASM_FAST_DIVMOD_H_

#include "backend__builder.h"
#include "common.h"

namespace syc {
namespace backend {

void fast_divmod(Builder& builder);

void fast_divmod_function(FunctionPtr function, Builder& builder);

void fast_divmod_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif