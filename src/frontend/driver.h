#ifndef SYC_FRONTEND_DRIVER_H_
#define SYC_FRONTEND_DRIVER_H_

#include "common.h"
#include "frontend/ast.h"
#include "frontend/comptime.h"
#include "frontend/generated/parser.h"

namespace syc {

namespace frontend {

class Parser;
class location;

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

  /// Stack of blocks.
  std::stack<ast::StmtPtr> block_stack;
  /// Count of temporary symbols.
  size_t next_temp_id;

  /// All tokens in formatted string.
  /// This is used mainly for debug.
  std::string tokens;
  /// Current type in declaration.
  TypePtr curr_decl_type;
  /// Current scope of declaration.
  Scope curr_decl_scope;
  /// If current decl is const.
  /// This is set by the lexer.
  bool is_curr_decl_const;

  /// Lexer generated by flex.
  yyscan_t lexer;
  /// Location for bison/flex
  location* loc;
  /// Parser generated by bison.
  Parser* parser;

  /// Constructor.
  Driver(std::string filename = "");
  /// Destructor.
  ~Driver();

  /// If the current scope if global.
  bool is_curr_global() const;

  /// Add a statement to the global/curr_block.
  void add_stmt(ast::StmtPtr stmt);

  /// Add a new block to the current context.
  /// This will create a new block and set it as the current block.
  void add_block();

  /// Quit from the current block
  void quit_block();

  /// Add a new function to the current context.
  /// This will create a new function and set it as the current function.
  /// Also, `add_block` will be called to create the body for the function.
  void add_function(
    TypePtr ret_type,
    std::string name,
    std::vector<std::tuple<TypePtr, std::string>> params
  );

  /// Quit from the current function and get back to the global context.
  void quit_function();

  void add_function_decl(
    TypePtr ret_type,
    std::string name,
    std::vector<std::tuple<TypePtr, std::string>> params
  );

  /// Add the token to `tokens`
  void add_token(const std::string& token);

  /// Get the name of the next temporary symbol entry.
  std::string get_next_temp_name();
};

}  // namespace frontend

}  // namespace syc

#endif