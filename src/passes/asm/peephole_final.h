#ifndef SYC_PASSES_ASM_PEEPHOLE_FINAL_H_
#define SYC_PASSES_ASM_PEEPHOLE_FINAL_H_

#include "backend/builder.h"
#include "common.h"

namespace syc {
namespace backend {

void peephole_final(Builder& builder);

void peephole_final_function(FunctionPtr function, Builder& builder);

void peephole_final_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif