#include "frontend/ast.h"

namespace syc {
namespace frontend {

namespace ast {

ExprPtr make_binary_expr(expr::BinaryOp op, ExprPtr lhs, ExprPtr rhs) {
  return std::make_shared<Expr>(ExprKind{expr::Binary{op, lhs, rhs}});
}

ExprPtr make_unary_expr(expr::UnaryOp op, ExprPtr expr) {
  return std::make_shared<Expr>(ExprKind{expr::Unary{op, expr}});
}

ExprPtr make_comptime_value_expr(std::variant<int, float> value) {
  return std::make_shared<Expr>(ExprKind{expr::ComptimeValue{value}});
}

ExprPtr make_cast_expr(ExprPtr expr, TypePtr type) {
  return std::make_shared<Expr>(ExprKind{expr::Cast{expr, type}});
}

}  // namespace ast

}  // namespace frontend

}  // namespace syc