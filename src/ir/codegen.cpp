#include "ir/codegen.h"
#include "backend/builder.h"
#include "ir/operand.h"

namespace syc {

void codegen(ir::Context& ir_context, AsmBuilder& builder) {
  // TODO
}

void codegen_function(IrFunctionPtr ir_function, AsmBuilder& builder) {
  // TODO
}

void codegen_basic_block(IrBasicBlockPtr ir_basic_block, AsmBuilder& builder) {
  // TODO
}

void codegen_instruction(IrInstructionPtr ir_instruction, AsmBuilder& builder) {
  // TODO
}

/// Perform register allocation.
void asm_register_allocation(AsmContext& context) {
  // TODO
}

/// Perform instruction scheduling.
void asm_instruction_scheduling(AsmContext& context) {
  // TODO
}

}  // namespace syc
