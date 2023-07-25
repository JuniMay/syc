#ifndef SYC_PASSES_IR_GLOBAL2LOCAL_H_
#define SYC_PASSES_IR_GLOBAL2LOCAL_H_

#include "common.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void global2local(Builder& builder);

}
}  // namespace syc

#endif  // SYC_PASSES_IR_GLOBAL2LOCAL_H_