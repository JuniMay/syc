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
  /// Symbol entry
  SymbolEntryPtr symbol;
};

/// Binary expression.
struct Binary {
  /// Operation
  BinaryOp op;
  /// Left-hand side expression.
  ExprPtr lhs;
  /// Right-hand side expression.
  ExprPtr rhs;
  /// Symbol entry of the destination.
  SymbolEntryPtr symbol;
};

/// Unary expression.
struct Unary {
  /// Operation
  UnaryOp op;
  /// Expression.
  ExprPtr expr;
  /// Symbol entry of the destination.
  SymbolEntryPtr symbol;
};

/// Initializer list for variable/constant definition.
struct InitializerList {
  /// values, represented as expressions.
  std::vector<ExprPtr> init_list;
  /// Type of the initializer list.
  TypePtr type;

  bool is_zeroinitializer = false;

  void set_type(TypePtr type, Driver& driver);

  void add_expr(ExprPtr expr);
};

/// Call expression.
struct Call {
  /// Function symbol.
  SymbolEntryPtr func_symbol;
  /// Arguments.
  std::vector<ExprPtr> args;
  /// Symbol entry for the destination.
  SymbolEntryPtr symbol;
};

/// Cast expression.
struct Cast {
  /// Expression to be casted.
  ExprPtr expr;
  /// Target type.
  TypePtr type;
  /// Symbol entry of the destination.
  SymbolEntryPtr symbol;
};

/// Literal number (or a constant)
struct Constant {
  /// Value of the literal.
  ComptimeValuePtr value;
};

}  // namespace expr

/// Expression.
struct Expr {
  /// Expression kind.
  ExprKind kind;

  /// Constructor
  Expr(ExprKind kind);

  /// If the expression is compile-time computable.
  bool is_comptime() const;

  /// Get the value.
  std::optional<ComptimeValuePtr> get_comptime_value() const;

  /// Get the type.
  TypePtr get_type() const;

  std::string to_string() const;

  std::string to_source_code(int depth=0) const;

  bool is_initializer_list() const;

  std::optional<expr::Binary> as_binary() const;

  bool operator==(const Expr& other) const;
};

std::pair<int, ExprPtr> as_integer_mul(ExprPtr expr);
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

  std::optional<stmt::Block> as_block() const;
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
ExprPtr create_constant_expr(ComptimeValuePtr value);

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
StmtPtr create_return_stmt(std::optional<ExprPtr> maybe_expr, Driver& driver);

/// Create a break statement.
StmtPtr create_break_stmt();

/// Create a continue statement.
StmtPtr create_continue_stmt();

/// Create an if statement.
StmtPtr create_if_stmt(
  ExprPtr cond,
  StmtPtr then_stmt,
  Driver& driver,
  std::optional<StmtPtr> maybe_else_stmt = std::nullopt
);

/// Create a while statement.
StmtPtr create_while_stmt(ExprPtr cond, StmtPtr body, Driver& driver);

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

StmtPtr create_assign_stmt(ExprPtr lhs, ExprPtr rhs, Driver& driver);

SymbolEntryPtr create_symbol_entry_from_decl_def(
  Scope scope,
  bool is_const,
  std::tuple<TypePtr, std::string, std::optional<ExprPtr>> def
);

}  // namespace ast

}  // namespace frontend

}  // namespace syc

#endif