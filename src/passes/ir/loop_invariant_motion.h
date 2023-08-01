#ifndef SYC_PASSES_IR_LOOP_INVARIANT_MOTION_H_
#define SYC_PASSES_IR_LOOP_INVARIANT_MOTION_H_

#include "common.h"
#include "passes/ir/loop_analysis.h"


namespace syc {
namespace ir {

void loop_invariant_motion(Builder& builder);

void loop_invariant_motion_function(Builder& builder, LoopOptContext& loop_opt_ctx);

}  // namespace ir
}  // namespace syc

#endif