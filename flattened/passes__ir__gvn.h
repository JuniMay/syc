#ifndef SYC_PASSES_IR_GVN_H_
#define SYC_PASSES_IR_GVN_H_

#include "common.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"
#include "passes__ir__control_flow_analysis.h"

namespace syc {
namespace ir {

void gvn(Builder& builder, bool is_aggressive);

void gvn_function(FunctionPtr function, Builder& builder, bool is_aggressive);

void gvn_basic_block(FunctionPtr function, BasicBlockPtr basic_block, Builder& builder, ControlFlowAnalysisContext cfa_ctx, bool is_aggressive);

}  // namespace ir
}  // namespace syc

#endif // SYC_PASSES_IR_GVN_H_