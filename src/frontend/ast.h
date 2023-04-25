#ifndef SYC_FRONTEND_AST_H_
#define SYC_FRONTEND_AST_H_

#include "common.h"

namespace syc {

namespace frontend {
namespace ast {

namespace expr {

/// Binary operations.
enum class BinaryOp {
  Add,
  Sub,
  Mul,
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
  LogicalAnd,
  LogicalOr,
};

/// Unary operations.
enum class UnaryOp {
  /// Positive
  Pos,
  /// Negative
  Neg,
  /// Not
  Not,
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

/// Call expression.
struct Call {
  /// Function name.
  std::string name;
  /// Arguments.
  std::vector<ExprPtr> args;
};

/// Compile-time value.
/// This is the representation of literal, constant, and compile time computed
/// result.
struct ComptimeValue {
  /// The value is either int or float.
  /// boolean is represented as int.
  std::variant<int, float> value;
};

/// Cast expression.
struct Cast {
  /// Expression to be casted.
  ExprPtr expr;
  /// Target type.
  TypePtr type;
};

/// Indexing expression.
struct Index {
  /// Expression to be indexed from.
  ExprPtr expr;
  /// Indexer expression.
  ExprPtr index;
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

struct If {
  ExprPtr cond;
  StmtPtr then;
  StmtPtr else_;
};

struct While {
  ExprPtr cond;
  StmtPtr body;
};

struct Break {};

struct Continue {};

struct Return {
  ExprPtr expr;
};

struct Assign {
  ExprPtr lhs;
  ExprPtr rhs;
};

struct Block {
  std::vector<StmtPtr> stmts;
};

struct Expr {
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
struct FuncDef {
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

struct Stmt {
  StmtKind kind;
};

ExprPtr make_binary_expr(expr::BinaryOp op, ExprPtr lhs, ExprPtr rhs);
ExprPtr make_unary_expr(expr::UnaryOp op, ExprPtr expr);
ExprPtr make_comptime_value_expr(std::variant<int, float> value);
ExprPtr make_cast_expr(ExprPtr expr, TypePtr type);

}  // namespace ast

}  // namespace frontend

}  // namespace syc

#endif