#ifndef SYC_FRONTEND_DRIVER_H_
#define SYC_FRONTEND_DRIVER_H_

#include "common.h"
#include "frontend/ast.h"

namespace syc {

namespace frontend {

/// Driver for parsing a program and generating the AST.
struct Driver {
  /// Root compile unit.
  ast::Compunit compunit;
  
  /// Current block.
  ast::StmtPtr curr_block;
  /// Current function.
  ast::StmtPtr curr_function;
  /// Current symbol table.
  SymbolTablePtr curr_symtable;

  /// Constructor.
  Driver();

  /// If the current scope if global.
  bool is_curr_global() const;

  /// Add a statement to the global/curr_block.
  void add_stmt(ast::StmtPtr stmt);
};
  
}

}

#endif