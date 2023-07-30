#ifndef SYC_PASSES_IR_STRENGTH_REDUCE_H_
#define SYC_PASSES_IR_STRENGTH_REDUCE_H_


#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"

namespace syc {
namespace ir {

void strength_reduce(Builder& builder);

void strength_reduce_function(FunctionPtr function, Builder& builder);

void strength_reduce_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc


#endif