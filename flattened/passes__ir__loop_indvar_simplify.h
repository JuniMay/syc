#ifndef SYC_PASSES_IR_LOOP_INDVAR_SIMPLIFY_H_
#define SYC_PASSES_IR_LOOP_INDVAR_SIMPLIFY_H_

#include "common.h"
#include "passes__ir__loop_analysis.h"

namespace syc {
namespace ir {

using BinaryOp = instruction::BinaryOp;

struct IvRecord {
  OperandID indvar_id;
  OperandID start_id;
  BinaryOp op;
  OperandID step_id;
};

void loop_indvar_simplify(Builder& builder);

void loop_indvar_simplify_function(FunctionPtr function, Builder& builder);

void loop_indvar_simplify_helper(LoopInfo& loop_info, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif