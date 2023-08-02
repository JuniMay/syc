#ifndef SYC_PASSES_IR_TCO_H_
#define SYC_PASSES_IR_TCO_H_

#include "ir/function.h"
#include "ir/builder.h"
#include "ir/instruction.h"
#include "ir/basic_block.h"
#include "ir/operand.h"
#include "common.h"

namespace syc {

namespace ir {

// Optimize tail call by replacing it with a jump instruction
void tco(
    Builder& builder
);

// Eliminate tail calls
void tail_call_elim_func(
    FunctionPtr function,
    Builder& builder
);

// Check if the instruction is a tail call)
bool is_inst_tail_call(
    InstructionPtr inst,
    FunctionPtr function,
    Builder& builder
);

}   // namespace ir

}  // namespace syc

#endif  // SYC_PASSES_IR_TCO_H_