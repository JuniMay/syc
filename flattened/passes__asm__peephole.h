#ifndef SYC_PASSES_ASM_PEEPHOLE_H_
#define SYC_PASSES_ASM_PEEPHOLE_H_

#include "common.h"

namespace syc {
namespace backend {

void peephole(Builder& builder);

void peephole_function(FunctionPtr function, Builder& builder);

void peephole_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif  // SYC_PASSES_ASM_PEEPHOLE_H_