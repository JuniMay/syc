#ifndef SYC_PASSES_IR_DCE_H_
#define SYC_PASSES_IR_DCE_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"

namespace syc {
namespace ir {

void dce(Builder& builder);

bool dce_function(FunctionPtr function, Builder& builder);

bool dce_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif  // SYC_PASSES_IR_DCE_H_