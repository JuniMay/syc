#ifndef SYC_PASSES_LOAD_ELIM_H_
#define SYC_PASSES_LOAD_ELIM_H_

#include "common.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void load_elim(Builder& builder);
void load_elim_function(FunctionPtr function, Builder& builder);
void load_elim_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif