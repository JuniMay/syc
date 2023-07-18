#ifndef SYC_PASSES_CSE_H_
#define SYC_PASSES_CSE_H_

#include "common.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void local_cse(Builder& builder);

void local_cse_function(FunctionPtr function, Builder& builder);

void local_cse_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif