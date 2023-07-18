#ifndef SYC_PASSES_ASM_PEEPHOLE_SECOND_H_
#define SYC_PASSES_ASM_PEEPHOLE_SECOND_H_

#include "common.h"

namespace syc {
namespace backend {

void peephole_second(Builder& builder);

void peephole_second_function(FunctionPtr function, Builder& builder);

void peephole_second_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif  // SYC_PASSES_ASM_PEEPHOLE_SECOND_H_