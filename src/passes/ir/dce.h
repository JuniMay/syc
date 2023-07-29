#ifndef SYC_PASSES_IR_DCE_H_
#define SYC_PASSES_IR_DCE_H_

#include "common.h"
#include "ir/builder.h"
#include "ir/function.h"

namespace syc {
namespace ir {

void dce(Builder& builder);

void dce_function(FunctionPtr function, Builder& builder);

void dce_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif  // SYC_PASSES_IR_DCE_H_