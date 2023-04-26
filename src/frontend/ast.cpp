#include "frontend/ast.h"
#include "frontend/symtable.h"
#include "frontend/type.h"

namespace syc {
namespace frontend {

ComptimeValue
create_comptime_value(std::variant<bool, int, float> value, TypePtr type) {
  return ComptimeValue{value, type};
}

ComptimeValue
comptime_compute_binary(BinaryOp op, ComptimeValue lhs, ComptimeValue rhs) {
  if (lhs.type != rhs.type) {
    throw std::runtime_error(
      "Inconsist type of compile-time values, consider implicit type cast."
    );
  }

  if (lhs.type->is_bool()) {
    switch (op) {
      case BinaryOp::LogicalAnd: {
        bool value = std::get<bool>(lhs.value) && std::get<bool>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::LogicalOr: {
        bool value = std::get<bool>(lhs.value) || std::get<bool>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time operation for type `bool`."
        );
      }
    }
  } else if (lhs.type->is_int()) {
    switch (op) {
      case BinaryOp::Add: {
        int value = std::get<int>(lhs.value) + std::get<int>(rhs.value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Sub: {
        int value = std::get<int>(lhs.value) - std::get<int>(rhs.value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Mul: {
        int value = std::get<int>(lhs.value) * std::get<int>(rhs.value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Mod: {
        int value = std::get<int>(lhs.value) % std::get<int>(rhs.value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Lt: {
        bool value = std::get<int>(lhs.value) < std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Gt: {
        bool value = std::get<int>(lhs.value) > std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Le: {
        bool value = std::get<int>(lhs.value) <= std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ge: {
        bool value = std::get<int>(lhs.value) >= std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Eq: {
        bool value = std::get<int>(lhs.value) == std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ne: {
        bool value = std::get<int>(lhs.value) != std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time operation for type `int`."
        );
      }
    }
  } else if (lhs.type->is_float()) {
    switch (op) {
      case BinaryOp::Add: {
        float value = std::get<float>(lhs.value) + std::get<float>(rhs.value);
        return create_comptime_value(value, create_float_type());
      }
      case BinaryOp::Sub: {
        float value = std::get<float>(lhs.value) - std::get<float>(rhs.value);
        return create_comptime_value(value, create_float_type());
      }
      case BinaryOp::Mul: {
        float value = std::get<float>(lhs.value) * std::get<float>(rhs.value);
        return create_comptime_value(value, create_float_type());
      }
      case BinaryOp::Lt: {
        bool value = std::get<int>(lhs.value) < std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Gt: {
        bool value = std::get<int>(lhs.value) > std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Le: {
        bool value = std::get<int>(lhs.value) <= std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ge: {
        bool value = std::get<int>(lhs.value) >= std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Eq: {
        bool value = std::get<int>(lhs.value) == std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ne: {
        bool value = std::get<int>(lhs.value) != std::get<int>(rhs.value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time operation for type `int`."
        );
      }
    }
  } else {
    throw std::runtime_error("Unsupported type for compile-time computation.");
  }
}

ComptimeValue comptime_compute_unary(UnaryOp op, ComptimeValue val) {
  if (op == UnaryOp::Pos) {
    // Note that if the operation is positive, the type of the value will not be
    // checked.
    return val;  // do nothing.
  }

  if (val.type->is_bool()) {
    switch (op) {
      case UnaryOp::Neg: {
        bool value = !std::get<bool>(val.value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time operation for type `bool`."
        );
      }
    }
  } else if (val.type->is_int()) {
    switch (op) {
      case UnaryOp::Neg: {
        int value = -std::get<int>(val.value);
        return create_comptime_value(value, create_int_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time operation for type `int`."
        );
      }
    }
  } else if (val.type->is_float()) {
    switch (op) {
      case UnaryOp::Neg: {
        float value = -std::get<float>(val.value);
        return create_comptime_value(value, create_float_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time operation for type `float`."
        );
      }
    }
  } else {
    throw std::runtime_error("Unsupported type for compile-time computation.");
  }
}

ComptimeValue comptime_compute_cast(ComptimeValue val, TypePtr type) {
  if (val.type == type) {
    return val;  // do nothing.
  }

  if (val.type->is_bool() && type->is_int()) {
    int value = static_cast<int>(std::get<bool>(val.value));
    return create_comptime_value(value, type);
  } else if (val.type->is_bool() && type->is_float()) {
    float value = static_cast<float>(std::get<bool>(val.value));
    return create_comptime_value(value, type);
  } else if (val.type->is_int() && type->is_bool()) {
    bool value = static_cast<bool>(std::get<int>(val.value));
    return create_comptime_value(value, type);
  } else if (val.type->is_int() && type->is_float()) {
    float value = static_cast<float>(std::get<int>(val.value));
    return create_comptime_value(value, type);
  } else if (val.type->is_float() && type->is_bool()) {
    bool value = static_cast<bool>(std::get<float>(val.value));
    return create_comptime_value(value, type);
  } else if (val.type->is_float() && type->is_int()) {
    int value = static_cast<int>(std::get<float>(val.value));
    return create_comptime_value(value, type);
  } else {
    throw std::runtime_error("Unsupported type for compile-time computation.");
  }
}

namespace ast {

Compunit::Compunit() : symtable(create_symbol_table(nullptr)), stmts({}) {}

ExprPtr create_binary_expr(BinaryOp op, ExprPtr lhs, ExprPtr rhs) {
  // TODO: binary expression may cause implicit type cast:
  //       1. int + float -> float
  //       2. using int/float in condition will cause implicit cast to bool
  //       3. maybe more...
}

ExprPtr create_unary_expr(UnaryOp op, ExprPtr expr) {
  // TODO: unary expression may cause implicit type cast:
  //       1. logical not may cause implicit cast to bool
  //       2. maybe more...
}

ExprPtr create_cast_expr(ExprPtr expr, TypePtr type) {
  return std::make_shared<Expr>(ExprKind(expr::Cast{expr, type}), type);
}

ExprPtr create_call_expr(const std::string& name, std::vector<ExprPtr> args) {
  // TODO: get the function symbol (and the type) from the corresponding symbol
  // table.
  return std::make_shared<Expr>(ExprKind(expr::Call{name, args}), nullptr);
}

void stmt::Block::add_stmt(StmtPtr stmt) {
  this->stmts.push_back(stmt);

  std::visit(
    overloaded{
      [this](const stmt::Decl& decl) {
        auto entry = decl.fetch_symbol_entry();
        this->symtable->add_symbol_entry(entry);
      },
      [](const auto& others) {
        // do nothing
      }},
    stmt->kind
  );
}

void Compunit::add_stmt(StmtPtr stmt) {
  this->stmts.push_back(stmt);

  std::visit(
    overloaded{
      [this](const stmt::Decl& decl) {
        auto entry = decl.fetch_symbol_entry();
        this->symtable->add_symbol_entry(entry);
      },
      [](const auto& others) {
        // do nothing
      }},
    stmt->kind
  );
}

SymbolEntryPtr stmt::Decl::fetch_symbol_entry() const {
  std::optional<ComptimeValue> value = std::nullopt;

  // Decide if the expression is a compile-time value.
  if (this->maybe_init.has_value()) {
    auto init_expr = this->maybe_init.value();
    if (std::holds_alternative<ComptimeValue>(init_expr->kind)) {
      value = std::get<ComptimeValue>(init_expr->kind);
    }
  }

  return create_symbol_entry(scope, name, type, is_const, value);
}

}  // namespace ast

}  // namespace frontend

}  // namespace syc