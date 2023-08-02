#ifndef SYC_PASSES_IR_GVN_H_
#define SYC_PASSES_IR_GVN_H_

#include "common.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "passes/ir/control_flow_analysis.h"

namespace syc {
namespace ir {

void gvn(Builder& builder, bool is_aggressive);

void gvn_function(FunctionPtr function, Builder& builder, bool is_aggressive);

void gvn_basic_block(FunctionPtr function, BasicBlockPtr basic_block, Builder& builder, ControlFlowAnalysisContext cfa_ctx, bool is_aggressive);

}  // namespace ir
}  // namespace syc

#endif // SYC_PASSES_IR_GVN_H_