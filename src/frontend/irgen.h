#ifndef SYC_FRONTEND_IRGEN_H_
#define SYC_FRONTEND_IRGEN_H_

#include "common.h"
#include "ir/builder.h"

namespace syc {

using AstBinaryOp = frontend::BinaryOp;
using AstUnaryOp = frontend::UnaryOp;

using AstCompunit = frontend::ast::Compunit;
using AstStmtPtr = frontend::ast::StmtPtr;
using AstExprPtr = frontend::ast::ExprPtr;

using AstTypePtr = frontend::TypePtr;
using AstSymbolEntryPtr = frontend::SymbolEntryPtr;
using AstSymbolTablePtr = frontend::SymbolTablePtr;
using AstComptimeValuePtr = frontend::ComptimeValuePtr;

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

IrOperandID
irgen_expr(AstExprPtr expr, AstSymbolTablePtr symtable, IrBuilder& builder);

std::optional<IrTypePtr> irgen_type(AstTypePtr type, IrBuilder& builder);

IrConstantPtr
irgen_comptime_value(AstComptimeValuePtr value, IrBuilder& builder);

}  // namespace syc

#endif