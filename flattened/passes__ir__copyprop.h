#ifndef SYC_PASSES_IR_COPYPROP_H_
#define SYC_PASSES_IR_COPYPROP_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__basic_block.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void copyprop(Builder& builder);

void copyprop_function(FunctionPtr function, Builder& builder);


}  // namespace ir
}  // namespace syc


#endif  // SYC_PASSES_IR_COPYPROP_H_