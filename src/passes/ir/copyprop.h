#ifndef SYC_PASSES_COPYPROP_H_
#define SYC_PASSES_COPYPROP_H_

#include "common.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void copyprop(Builder& builder);

void copyprop_function(FunctionPtr function, Builder& builder);


}  // namespace ir
}  // namespace syc


#endif  // SYC_PASSES_COPYPROP_H_