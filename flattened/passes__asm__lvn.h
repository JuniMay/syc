#ifndef SYC_PASSES_ASM_LVN_H_
#define SYC_PASSES_ASM_LVN_H_

#include "backend__builder.h"
#include "common.h"

namespace syc {
namespace backend {

void lvn(Builder& builder);

void lvn_function(FunctionPtr function, Builder& builder);

void lvn_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif