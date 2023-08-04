#ifndef SYC_PASSES_IR_LOOP_UNROLLING_H_
#define SYC_PASSES_IR_LOOP_UNROLLING_H_

#include "common.h"
#include "passes/ir/loop_analysis.h"

namespace syc {
namespace ir {

struct LoopUnrollingContext {
  std::map<OperandID, OperandID> operand_id_map;
  std::map<BasicBlockID, BasicBlockID> basic_block_id_map;
  LoopInfo loop_info;

  LoopUnrollingContext() = default;
};

void loop_unrolling(Builder& builder);

void loop_unrolling_function(FunctionPtr function, Builder& builder);

bool loop_unrolling_helper(LoopInfo& loop_info, Builder& builder);

OperandID clone_operand(
  OperandID operand_id,
  Builder& builder,
  LoopUnrollingContext& context
);

InstructionPtr clone_instruction(
  InstructionPtr instruction,
  Builder& builder,
  LoopUnrollingContext& context
);

}  // namespace ir
}  // namespace syc

#endif