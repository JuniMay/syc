#ifndef SYC_IR_CODEGEN_H_
#define SYC_IR_CODEGEN_H_

#include "backend/builder.h"
#include "backend/context.h"
#include "common.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {

using IrOperandID = ir::OperandID;
using IrContext = ir::Context;
using IrInstructionPtr = ir::InstructionPtr;
using IrBasicBlockPtr = ir::BasicBlockPtr;
using IrFunctionPtr = ir::FunctionPtr;

using AsmOperandID = backend::OperandID;
using AsmBuilder = backend::Builder;
using AsmContext = backend::Context;
using AsmInstructionPtr = backend::InstructionPtr;
using AsmBasicBlockPtr = backend::BasicBlockPtr;
using AsmFunctionPtr = backend::FunctionPtr;

struct CodegenContext {
  std::map<IrOperandID, AsmOperandID> operand_map;

  CodegenContext() = default;
};

void codegen(
  ir::Context& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_function(
  IrFunctionPtr ir_function,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_basic_block(
  IrBasicBlockPtr ir_basic_block,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_instruction(
  IrInstructionPtr ir_instruction,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

/// Perform register allocation.
void asm_register_allocation(AsmContext& context);

/// Perform instruction scheduling.
void asm_instruction_scheduling(AsmContext& context);

}  // namespace syc

#endif