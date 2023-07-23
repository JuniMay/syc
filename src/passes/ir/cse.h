#ifndef SYC_PASSES_IR_CSE_H_
#define SYC_PASSES_IR_CSE_H_

#include "common.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void local_cse(Builder& builder);

void local_cse_function(FunctionPtr function, Builder& builder);

void local_cse_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif