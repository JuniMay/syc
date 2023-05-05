#ifndef SYC_FRONTEND_IRGEN_H_
#define SYC_FRONTEND_IRGEN_H_

#include "common.h"
#include "frontend/ast.h"
#include "ir/builder.h"

namespace syc {

void irgen_compunit(
  frontend::ast::Compunit& ast_compunit,
  ir::Builder& ir_builder
);

void irgen_expr(frontend::ast::ExprPtr ast_expr, ir::Builder& ir_builder);

void irgen_stmt(frontend::ast::StmtPtr ast_stmt, ir::Builder& ir_builder);

ir::TypePtr irgen_type(frontend::TypePtr ast_type, ir::Builder& ir_builder);

ir::OperandID irgen_symbol_entry(
  frontend::SymbolEntryPtr ast_symbol_entry,
  ir::Builder& ir_builder
);

ir::OperandID irgen_comtime_value(
  frontend::ComptimeValuePtr ast_comptime_value,
  ir::Builder& ir_builder
);

}  // namespace syc

#endif