#ifndef SYC_PASSES_IR_UNREACH_ELIM_H_
#define SYC_PASSES_IR_UNREACH_ELIM_H_

#include "ir/function.h"
#include "ir/builder.h"
#include "ir/instruction.h"
#include "ir/basic_block.h"
#include "ir/operand.h"
#include "common.h"

namespace syc {

namespace ir {

void unreach_elim(
    Builder& builder
);

/// Traverse branch instructions with constant condition 
/// and transform them into uncond branch instructions
void elim_const_branch(
    FunctionPtr function, 
    Builder& builder
);

/// Remove unreachable basic blocks
void remove_unreach_block(
    FunctionPtr function, 
    Builder& builder
);

/// Traverse the function and return all reachable blocks
std::set<BasicBlockID> get_reachable_blocks(
    FunctionPtr function,
    Builder& builder
);

}   // namespace ir

}  // namespace syc

#endif  // SYC_PASSES_IR_UNREACH_ELIM_H_