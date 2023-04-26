#ifndef SYC_FRONTEND_AST_H_
#define SYC_FRONTEND_AST_H_

#include "common.h"

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

/// Compile-time value.
/// This is the representation of literal, constant, and compile time computed
/// result.
struct ComptimeValue {
  /// The value is bool, int or float.
  std::variant<bool, int, float> value;
  TypePtr type;
};

ComptimeValue
create_comptime_value(std::variant<bool, int, float> value, TypePtr type);

/// Compute binary operation between two compile-time values.
ComptimeValue
comptime_compute_binary(BinaryOp op, ComptimeValue lhs, ComptimeValue rhs);

/// Compute unary operation on a compile-time value.
ComptimeValue comptime_compute_unary(UnaryOp op, ComptimeValue val);

/// Compute cast operation on a compile-time value.
ComptimeValue comptime_compute_cast(ComptimeValue val, TypePtr type);

namespace ast {

namespace expr {

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

}  // namespace expr

/// Expression.
struct Expr {
  /// Expression kind.
  ExprKind kind;
  /// Expression type.
  TypePtr type;
};

namespace stmt {

/// If statement.
struct If {
  /// Condition
  ExprPtr cond;
  /// True branch
  StmtPtr then_stmt;
  /// False branch
  StmtPtr else_stmt;
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
  ExprPtr expr;
};

/// Block statement.
struct Block {
  /// Symbol table of the block.
  SymbolTablePtr symtable;
  /// Statements.
  std::vector<StmtPtr> stmts;
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

}  // namespace stmt

/// Declaration.
/// This is used for both global and local.
struct Decl {
  /// If the declaration is a constant.
  bool is_const;
  /// Type of the declaration.
  TypePtr type;
  /// Name of the declaration.
  std::string name;
  /// Indices (of array) in the declaration.
  std::optional<std::vector<ExprPtr>> indices;
  /// Initial value.
  std::optional<ExprPtr> init;
};

/// Function definition.
struct Func {
  /// Symbol table
  /// In this symbol table, only parameters are included,
  /// and other symbols are stored in the block statement.
  SymbolTablePtr symtable;
  /// Return type.
  TypePtr ret_type;
  /// Function name.
  std::string name;
  /// Parameters.
  /// The `init` in each Decl is nullopt.
  std::vector<Decl> params;
  /// A block statement.
  StmtPtr body;
};

/// Statement.
struct Stmt {
  /// Statement kind.
  StmtKind kind;
};

/// Compile unit.
struct CompUnit {
  /// Global symbols.
  SymbolTablePtr symtable;
  /// Items.
  std::vector<std::variant<Decl, Func>> items;

  /// Constructor.
  CompUnit();
};

ExprPtr create_binary_expr(BinaryOp op, ExprPtr lhs, ExprPtr rhs);
ExprPtr create_unary_expr(UnaryOp op, ExprPtr expr);
ExprPtr create_cast_expr(ExprPtr expr, TypePtr type);
ExprPtr create_call_expr(const std::string& name, std::vector<ExprPtr> args);

}  // namespace ast

}  // namespace frontend

}  // namespace syc

#endif