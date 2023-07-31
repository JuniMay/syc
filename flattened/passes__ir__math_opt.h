#ifndef SYC_PASSES_IR_MATH_OPT_H_
#define SYC_PASSES_IR_MATH_OPT_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"

namespace syc {
namespace ir {
struct MathOptContext {
  using BinaryOp = instruction::BinaryOp;

  std::unordered_map<OperandID, std::tuple<BinaryOp, OperandID, OperandID>>
    expr_forest;

  MathOptContext() = default;
};

void math_opt(Builder& builder);

void math_opt_function(FunctionPtr function, Builder& builder);

void math_opt_block(BasicBlockPtr basic_block, Builder& builder, MathOptContext& math_opt_ctx);

bool simplify(Builder& builder, MathOptContext& math_opt_ctx);

void prune(Builder& builder, MathOptContext& math_opt_ctx);

}  // namespace ir
}  // namespace syc

#endif  // SYC_PASSSES_IR_MATH_OPT_H_