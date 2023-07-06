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
#include "passes/linear_scan.h"

namespace syc {

using IrOperandID = ir::OperandID;
using IrBasicBlockID = ir::BasicBlockID;
using IrContext = ir::Context;
using IrInstructionPtr = ir::InstructionPtr;
using IrBasicBlockPtr = ir::BasicBlockPtr;
using IrFunctionPtr = ir::FunctionPtr;

using AsmOperandID = backend::OperandID;
using AsmOperandPtr = backend::OperandPtr;
using AsmBuilder = backend::Builder;
using AsmContext = backend::Context;
using AsmBasicBlockID = backend::BasicBlockID;
using AsmInstructionPtr = backend::InstructionPtr;
using AsmBasicBlockPtr = backend::BasicBlockPtr;
using AsmFunctionPtr = backend::FunctionPtr;

struct CodegenContext {
  std::map<IrOperandID, AsmOperandID> operand_map;
  std::map<IrBasicBlockID, AsmBasicBlockID> basic_block_map;

  CodegenContext() = default;

  AsmOperandID get_asm_operand_id(IrOperandID ir_operand_id);
};

void codegen(
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_function(
  IrFunctionPtr ir_function,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_function_prolouge(
  std::string function_name,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_function_epilouge(
  std::string function_name,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_basic_block(
  IrBasicBlockPtr ir_basic_block,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

void codegen_instruction(
  IrInstructionPtr ir_instruction,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
);

/// Generate corresponding asm operan for ir operand.
/// try_keep_imm: if the immediate is an i-type immediate, keep it as immediate,
///   otherwise load it into a register.
/// use_fmv: fmv floating-point bits from general register to a float register.
AsmOperandID codegen_operand(
  IrOperandID ir_operand,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context,
  bool try_keep_imm = false,
  bool use_fmv = false
);

bool check_utype_immediate(uint32_t value);

bool check_itype_immediate(int32_t value);

/// Perform register allocation.
void asm_register_allocation(AsmBuilder& builder);

/// Perform instruction scheduling.
void asm_instruction_scheduling(AsmContext& context);

}  // namespace syc

#endif