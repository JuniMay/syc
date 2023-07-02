#include "backend/codegen.h"
#include "ir/operand.h"
#include "backend/builder.h"

namespace syc {

namespace backend {

void codegen(ir::Context& ir_context, Builder& builder) {
  
}

/// Select instruction from IR instruction.
void select_instruction(ir::Instruction& ir_instruction, Context& context);

/// Perform register allocation.
void register_allocation(Context& context);

/// Perform instruction scheduling.
// void instruction_scheduling(Context& context);

/// Emit assembly code from the context.
void emit_assembly(std::ostream& out, Context& context);

}  // namespace backend
}  // namespace syc
