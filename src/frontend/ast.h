#ifndef SYC_FRONTEND_AST_H_
#define SYC_FRONTEND_AST_H_

#include "common.h"
#include "frontend/comptime.h"

namespace syc {

namespace frontend {

/// Binary operations.
enum class BinaryOp {
  /// Add
  Add,
  /// Sub
  Sub,
  /// Mul
  Mul,
  /// Div
  Div,
  /// Mod
  Mod,
  /// Less than.
  Lt,
  /// Greater than.
  Gt,
  /// Less than or equal to.
  Le,
  /// Greater than or equal to.
  Ge,
  /// Equal to.
  Eq,
  /// Not equal to.
  Ne,
  /// Logical And
  /// This is for bool type.
  LogicalAnd,
  /// Logical Or
  /// This is for bool type.
  LogicalOr,
  /// Index
  Index,
};

/// Unary operations.
enum class UnaryOp {
  /// Positive
  Pos,
  /// Negative
  Neg,
  /// LogicalNot
  LogicalNot,
};

namespace ast {

namespace expr {

/// Identifier as an expression.
struct Identifier {
  /// Name of the identifier
  std::string name;
};

/// Binary expression.
struct Binary {
  /// Operation
  BinaryOp op;
  /// Left-hand side expression.
  ExprPtr lhs;
  /// Right-hand side expression.
  ExprPtr rhs;
};

/// Unary expression.
struct Unary {
  /// Operation
  UnaryOp op;
  /// Expression.
  ExprPtr expr;
};

/// Initializer list for variable/constant definition.
struct InitializerList {
  /// values, represented as expressions.
  std::vector<ExprPtr> init_list;
  /// Type of the initializer list.
  std::optional<TypePtr> maybe_type;

  void set_type(TypePtr);
};

/// Call expression.
struct Call {
  /// Function name.
  std::string name;
  /// Arguments.
  std::vector<ExprPtr> args;
};

/// Cast expression.
struct Cast {
  /// Expression to be casted.
  ExprPtr expr;
  /// Target type.
  TypePtr type;
};

/// Literal number (or a constant)
struct Constant {
  /// Value of the literal.
  ComptimeValue value;
};

}  // namespace expr

/// Expression.
struct Expr {
  /// Expression kind.
  ExprKind kind;
  /// Symbol entry
  /// If the expression is a identifier, the symbol entry is corresponding to
  /// the name. Otherwise the symbol entry is temporary.
  /// If the expression is a literal value, the symbol entry is
  /// nullopt.
  /// TODO: type for initializer list. The expression in init list is not always
  /// the same type.
  std::optional<SymbolEntryPtr> maybe_symbol_entry;

  /// Constructor
  Expr(ExprKind kind, std::optional<SymbolEntryPtr> maybe_symbol_entry);

  /// If the expression is compile-time computable.
  bool is_comptime() const;

  /// Get the value.
  std::optional<ComptimeValue> get_comptime_value() const;

  /// Get the type.
  TypePtr get_type() const;

  std::string to_string() const;

  bool is_initializer_list() const;
};

namespace stmt {

struct Blank {};

/// If statement.
struct If {
  /// Condition
  ExprPtr cond;
  /// True branch
  StmtPtr then_stmt;
  /// False branch, nullopt if no else statement.
  std::optional<StmtPtr> maybe_else_stmt;
};

/// While statement.
struct While {
  /// Condition
  ExprPtr cond;
  /// Body
  StmtPtr body;
};

/// Break
struct Break {};

/// Continue
struct Continue {};

/// Return
struct Return {
  /// Return expression.
  std::optional<ExprPtr> maybe_expr;
};

/// Block statement.
struct Block {
  /// Symbol table of the block.
  SymbolTablePtr symtable;
  /// Statements.
  std::vector<StmtPtr> stmts;

  void add_stmt(StmtPtr stmt);
};

/// Assign statement
struct Assign {
  /// Left-hand side expression.
  /// This can be a identifier or an index expression.
  ExprPtr lhs;
  /// Right-hand side expression.
  ExprPtr rhs;
};

/// Expression statement.
struct Expr {
  /// The expression.
  ExprPtr expr;
};

/// Declaration.
/// This is used for both global and local.
struct Decl {
  /// Scope of the variable/constant.
  Scope scope;
  /// If the declaration is a constant.
  bool is_const;
  /// A declaration may contain multiple definitions.
  std::vector<std::tuple<TypePtr, std::string, std::optional<ExprPtr>>> defs;

  /// Get the number of definitions.
  size_t get_def_cnt() const;
  /// Get the symbol entry.
  SymbolEntryPtr fetch_symbol_entry(size_t idx) const;
};

/// Function definition.
struct FuncDef {
  /// Symbol table
  /// In this symbol table, only parameters are included,
  /// and other symbols are stored in the block statement.
  SymbolTablePtr symtable;
  /// Symbol entry of the function
  SymbolEntryPtr symbol_entry;
  /// Parameter names.
  std::vector<std::string> param_names;
  /// A block statement.
  /// If the body is nullopt, the function is a declaration.
  /// e.g. functions in the runtime library.
  std::optional<StmtPtr> maybe_body;

  void set_body(StmtPtr body);
};

}  // namespace stmt

/// Statement.
struct Stmt {
  /// Statement kind.
  StmtKind kind;

  Stmt(StmtKind kins);

  std::string to_string() const;
};

/// Compile unit.
struct Compunit {
  /// Global symbols.
  SymbolTablePtr symtable;
  /// Items.
  std::vector<StmtPtr> stmts;
  /// Constructor.
  Compunit();

  /// Add a statement to the compile unit.
  /// If the statement is a declaration, the symbol entry will be registered.
  void add_stmt(StmtPtr stmt);

  std::string to_string() const;
};

/// Create an identifier expression from the corresponding symbol entry.
/// When building the AST, symbol_entry can be looked up in the `curr_symtable`
/// by driver using its name.
ExprPtr create_identifier_expr(SymbolEntryPtr symbol_entry);

/// Create a constant expression.
ExprPtr create_constant_expr(ComptimeValue value);

/// Create a binary expression.
/// `symbol_name` is the temporary name fetched from the driver.
/// `symtable` if the symbol table to register the temporary symbol entry.
ExprPtr
create_binary_expr(BinaryOp op, ExprPtr lhs, ExprPtr rhs, Driver& driver);

/// Create a unary expression.
/// `symbol_name` is the temporary name fetched from the driver.
/// `symtable` if the symbol table to register the temporary symbol entry.
ExprPtr create_unary_expr(UnaryOp op, ExprPtr expr, Driver& driver);

/// Create a call expression.
/// `symbol_name` is the temporary name fetched from the driver.
/// `symtable` if the symbol table to register the temporary symbol entry.
/// Note that the type of the symbol entry can be void.
ExprPtr create_call_expr(
  SymbolEntryPtr func_symbol_entry,
  std::vector<ExprPtr> args,
  Driver& driver
);

ExprPtr create_cast_expr(ExprPtr expr, TypePtr type, Driver& driver);

ExprPtr create_initializer_list_expr(std::vector<ExprPtr> init_list);

StmtPtr create_blank_stmt();

/// Create a return statement.
StmtPtr create_return_stmt(std::optional<ExprPtr> maybe_expr = std::nullopt);

/// Create a break statement.
StmtPtr create_break_stmt();

/// Create a continue statement.
StmtPtr create_continue_stmt();

/// Create an if statement.
StmtPtr create_if_stmt(
  ExprPtr cond,
  StmtPtr then_stmt,
  std::optional<StmtPtr> maybe_else_stmt = std::nullopt
);

/// Create a while statement.
StmtPtr create_while_stmt(ExprPtr cond, StmtPtr body);

/// Create a new block with no statements.
StmtPtr create_block_stmt(SymbolTablePtr parent_symtable);

/// Create a new function definition with no statements.
/// This will not create a new block. The body is handled by the driver.
StmtPtr create_func_def_stmt(
  SymbolTablePtr parent_symtable,
  TypePtr ret_type,
  std::string name,
  std::vector<std::tuple<TypePtr, std::string>> params
);

/// Create a declaration statement.
StmtPtr create_decl_stmt(
  Scope scope,
  bool is_const,
  std::vector<std::tuple<TypePtr, std::string, std::optional<ExprPtr>>> defs
);

StmtPtr create_expr_stmt(ExprPtr expr);

StmtPtr create_assign_stmt(ExprPtr lhs, ExprPtr rhs);

}  // namespace ast

}  // namespace frontend

}  // namespace syc

#endif