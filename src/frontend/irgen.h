#ifndef SYC_FRONTEND_IRGEN_H_
#define SYC_FRONTEND_IRGEN_H_

#include "common.h"
#include "frontend/ast.h"
#include "frontend/symtable.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {

using AstBinaryOp = frontend::BinaryOp;
using AstUnaryOp = frontend::UnaryOp;

using AstCompunit = frontend::ast::Compunit;
using AstStmtPtr = frontend::ast::StmtPtr;
using AstExprPtr = frontend::ast::ExprPtr;

using AstTypePtr = frontend::TypePtr;
using AstScope = frontend::Scope;
using AstSymbolEntryPtr = frontend::SymbolEntryPtr;
using AstSymbolTablePtr = frontend::SymbolTablePtr;
using AstComptimeValuePtr = frontend::ComptimeValuePtr;

using IrBinaryOp = ir::instruction::BinaryOp;
using IrICmpCond = ir::instruction::ICmpCond;
using IrFCmpCond = ir::instruction::FCmpCond;
using IrCastOp = ir::instruction::CastOp;

using IrBuilder = ir::Builder;
using IrTypePtr = ir::TypePtr;
using IrConstantPtr = ir::operand::ConstantPtr;
using IrOperandID = ir::OperandID;
using IrInstructionPtr = ir::InstructionPtr;
using IrBasicBlockPtr = ir::BasicBlockPtr;

void irgen(const AstCompunit& compunit, IrBuilder& builder);

void irgen_stmt(
  AstStmtPtr stmt,
  AstSymbolTablePtr symtable,
  IrBuilder& builder
);

/// Generate IR for expression.
/// `use_address` is used to decide if the value shall be loaded from the
/// address or just get a pointer.
std::optional<IrOperandID> irgen_expr(
  AstExprPtr expr,
  AstSymbolTablePtr symtable,
  IrBuilder& builder,
  bool use_address = false
);

std::optional<IrTypePtr> irgen_type(AstTypePtr type, IrBuilder& builder);

IrConstantPtr
irgen_comptime_value(AstComptimeValuePtr value, IrBuilder& builder);

}  // namespace syc

#endif