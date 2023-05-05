#include "frontend/comptime.h"
#include "frontend/ast.h"
#include "utils.h"

namespace syc {
namespace frontend {

std::string ComptimeValue::to_string() const {
  if (this->is_zeroinitializer()) {
    return "COMPTIME ZEROINITIALIZER";
  }

  std::stringstream buf;

  if (this->type->is_bool()) {
    buf << "COMPTIME BOOL " << std::get<bool>(this->value);
  } else if (this->type->is_int()) {
    buf << "COMPTIME INT " << std::get<int>(this->value);
  } else if (this->type->is_float()) {
    buf << "COMPTIME FLOAT " << std::get<float>(this->value);
  } else if (this->type->is_array()) {
    buf << "COMPTIME ARRAY {" << std::endl;
    for (auto& item : std::get<std::vector<ComptimeValuePtr>>(this->value)) {
      buf << indent_str(item->to_string(), "\t") << std::endl;
    }
    buf << "}";
  } else {
    throw std::runtime_error(
      "Unsupported type for compile-time value to be converted to string."
    );
  }

  return buf.str();
}

bool ComptimeValue::is_zeroinitializer() const {
  return std::holds_alternative<Zeroinitializer>(this->value);
}

ComptimeValuePtr create_comptime_value(ComptimeValueKind value, TypePtr type) {
  return std::make_shared<ComptimeValue>(ComptimeValue{value, type});
}

ComptimeValuePtr create_zero_comptime_value(TypePtr type) {
  if (type->is_int()) {
    return create_comptime_value((int)0, type);
  } else if (type->is_float()) {
    return create_comptime_value((float)0., type);
  } else if (type->is_bool()) {
    return create_comptime_value(false, type);
  } else if (type->is_array()) {
    return create_comptime_value(Zeroinitializer{}, type);
  } else {
    // Not knowing which type to store.
    return create_comptime_value(0, type);
  }
}

ComptimeValuePtr comptime_compute_binary(
  BinaryOp op,
  ComptimeValuePtr lhs,
  ComptimeValuePtr rhs
) {
  if (lhs->type->is_bool()) {
    switch (op) {
      case BinaryOp::LogicalAnd: {
        bool value = std::get<bool>(lhs->value) && std::get<bool>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::LogicalOr: {
        bool value = std::get<bool>(lhs->value) || std::get<bool>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time binary operation for type `bool`."
        );
      }
    }
  } else if (lhs->type->is_int()) {
    switch (op) {
      case BinaryOp::Add: {
        int value = std::get<int>(lhs->value) + std::get<int>(rhs->value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Sub: {
        int value = std::get<int>(lhs->value) - std::get<int>(rhs->value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Mul: {
        int value = std::get<int>(lhs->value) * std::get<int>(rhs->value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Div: {
        int value = std::get<int>(lhs->value) / std::get<int>(rhs->value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Mod: {
        int value = std::get<int>(lhs->value) % std::get<int>(rhs->value);
        return create_comptime_value(value, create_int_type());
      }
      case BinaryOp::Lt: {
        bool value = std::get<int>(lhs->value) < std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Gt: {
        bool value = std::get<int>(lhs->value) > std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Le: {
        bool value = std::get<int>(lhs->value) <= std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ge: {
        bool value = std::get<int>(lhs->value) >= std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Eq: {
        bool value = std::get<int>(lhs->value) == std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ne: {
        bool value = std::get<int>(lhs->value) != std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time binary operation for type `int`."
        );
      }
    }
  } else if (lhs->type->is_float()) {
    switch (op) {
      case BinaryOp::Add: {
        float value = std::get<float>(lhs->value) + std::get<float>(rhs->value);
        return create_comptime_value(value, create_float_type());
      }
      case BinaryOp::Sub: {
        float value = std::get<float>(lhs->value) - std::get<float>(rhs->value);
        return create_comptime_value(value, create_float_type());
      }
      case BinaryOp::Mul: {
        float value = std::get<float>(lhs->value) * std::get<float>(rhs->value);
        return create_comptime_value(value, create_float_type());
      }
      case BinaryOp::Lt: {
        bool value = std::get<int>(lhs->value) < std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Gt: {
        bool value = std::get<int>(lhs->value) > std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Le: {
        bool value = std::get<int>(lhs->value) <= std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ge: {
        bool value = std::get<int>(lhs->value) >= std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Eq: {
        bool value = std::get<int>(lhs->value) == std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      case BinaryOp::Ne: {
        bool value = std::get<int>(lhs->value) != std::get<int>(rhs->value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time binary operation for type `int`."
        );
      }
    }
  } else if (lhs->type->is_array() && rhs->type->is_int()) {
    auto element_type = lhs->type->get_element_type().value();
    if (lhs->is_zeroinitializer()) {
      return create_zero_comptime_value(element_type);
    } else {
      auto array = std::get<std::vector<ComptimeValuePtr>>(lhs->value);
      auto index = std::get<int>(rhs->value);
      return array[index];
    }
  } else {
    throw std::runtime_error(
      "Unsupported type for compile-time bianry operation."
    );
  }
}

ComptimeValuePtr comptime_compute_unary(UnaryOp op, ComptimeValuePtr val) {
  if (op == UnaryOp::Pos) {
    // Note that if the operation is positive, the type of the value will not be
    // checked.
    return val;  // do nothing.
  }

  if (val->type->is_bool()) {
    switch (op) {
      case UnaryOp::Neg: {
        bool value = !std::get<bool>(val->value);
        return create_comptime_value(value, create_bool_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time unary operation for type `bool`."
        );
      }
    }
  } else if (val->type->is_int()) {
    switch (op) {
      case UnaryOp::Neg: {
        int value = -std::get<int>(val->value);
        return create_comptime_value(value, create_int_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time unary operation for type `int`."
        );
      }
    }
  } else if (val->type->is_float()) {
    switch (op) {
      case UnaryOp::Neg: {
        float value = -std::get<float>(val->value);
        return create_comptime_value(value, create_float_type());
      }
      default: {
        throw std::runtime_error(
          "Unsupported compile-time unary operation for type `float`."
        );
      }
    }
  } else {
    throw std::runtime_error(
      "Unsupported type for compile-time unary operation."
    );
  }
}

ComptimeValuePtr comptime_compute_cast(ComptimeValuePtr val, TypePtr type) {
  if (val->type == type) {
    return val;  // do nothing.
  }

  if (val->type->is_bool() && type->is_int()) {
    int value = static_cast<int>(std::get<bool>(val->value));
    return create_comptime_value(value, type);
  } else if (val->type->is_bool() && type->is_float()) {
    float value = static_cast<float>(std::get<bool>(val->value));
    return create_comptime_value(value, type);
  } else if (val->type->is_int() && type->is_bool()) {
    bool value = static_cast<bool>(std::get<int>(val->value));
    return create_comptime_value(value, type);
  } else if (val->type->is_int() && type->is_float()) {
    float value = static_cast<float>(std::get<int>(val->value));
    return create_comptime_value(value, type);
  } else if (val->type->is_float() && type->is_bool()) {
    bool value = static_cast<bool>(std::get<float>(val->value));
    return create_comptime_value(value, type);
  } else if (val->type->is_float() && type->is_int()) {
    int value = static_cast<int>(std::get<float>(val->value));
    return create_comptime_value(value, type);
  } else {
    throw std::runtime_error("Unsupported type for compile-time computation.");
  }
}

}  // namespace  frontend

}  // namespace syc