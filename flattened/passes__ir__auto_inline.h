#ifndef SYC_PASSES_IR_AUTO_INLINE_H_
#define SYC_PASSES_IR_AUTO_INLINE_H_

#include "common.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

struct AutoInlineContext {
  std::map<OperandID, OperandID> operand_id_map;
  std::map<BasicBlockID, BasicBlockID> basic_block_id_map;

  AutoInlineContext() = default;
};

void auto_inline(Builder& builder);

void auto_inline_instruction(InstructionPtr instruction, Builder& builder);

OperandID clone_operand(
  OperandID operand_id,
  Builder& builder,
  AutoInlineContext& context
);

InstructionPtr clone_instruction(
  InstructionPtr instruction,
  Builder& builder,
  AutoInlineContext& context
);

}  // namespace ir
}  // namespace syc

#endif