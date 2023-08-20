#ifndef SYC_PASSES_IR_FUNC_RET_OPT_H_
#define SYC_PASSES_IR_FUNC_RET_OPT_H_

#include "common.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void func_ret_opt(Builder& builder);

}
}  // namespace syc

#endif