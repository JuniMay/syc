#ifndef SYC_BACKEND_CODEGEN_H_
#define SYC_BACKEND_CODEGEN_H_

#include "common.h"
#include "ir/context.h"
#include "backend/context.h"

namespace syc {

namespace backend {

void codegen(ir::Context& ir_context, Builder& builder); 

/// Select instruction from IR instruction.
void select_instruction(ir::Instruction& ir_instruction, Context& context);

/// Perform register allocation.
void register_allocation(Context& context); 

/// Perform instruction scheduling.
// void instruction_scheduling(Context& context);

/// Emit assembly code from the context.
void emit_assembly(std::ostream& out, Context& context);

} // namespace backend 

} // namespace syc

#endif