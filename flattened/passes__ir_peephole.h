#ifndef SYC_PASSES_IR_PEEPHOLE_H_
#define SYC_PASSES_IR_PEEPHOLE_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"

namespace syc {
namespace ir {

void peephole(Builder& builder);

void peephole_function(FunctionPtr function, Builder& builder);

void peephole_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif  // SYC_PASSES_IR_PEEPHOLE_H_