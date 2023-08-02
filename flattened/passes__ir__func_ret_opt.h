#ifndef SYC_PASSES_IR_FUNC_RET_OPT_H_
#define SYC_PASSES_IR_FUNC_RET_OPT_H_

#include "common.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void func_ret_opt(Builder& builder);

}
}  // namespace syc

#endif