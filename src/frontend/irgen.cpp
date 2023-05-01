#include "irgen.h"

namespace syc {

void irgen_compunit(
  frontend::ast::Compunit& ast_compunit,
  ir::Builder& ir_builder
) {}

void irgen_expr(frontend::ast::ExprPtr ast_expr, ir::Builder& ir_builder) {}

void irgen_stmt(frontend::ast::StmtPtr ast_stmt, ir::Builder& ir_builder) {}

}  // namespace syc