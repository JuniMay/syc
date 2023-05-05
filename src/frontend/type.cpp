#include "frontend/type.h"
#include "frontend/ast.h"

namespace syc {
namespace frontend {

Type::Type(TypeKind kind) : kind(kind) {}

bool Type::is_bool() const {
  return std::holds_alternative<type::Integer>(kind) &&
         std::get<type::Integer>(kind).size == 1;
}

bool Type::is_int() const {
  return std::holds_alternative<type::Integer>(kind) &&
         std::get<type::Integer>(kind).size == 32;
}

bool Type::is_float() const {
  return std::holds_alternative<type::Float>(kind);
}

bool Type::is_array() const {
  return std::holds_alternative<type::Array>(kind);
}

bool Type::is_void() const {
  return std::holds_alternative<type::Void>(kind);
}

bool Type::is_pointer() const {
  return std::holds_alternative<type::Pointer>(kind);
}

bool Type::is_function() const {
  return std::holds_alternative<type::Function>(kind);
}

std::optional<TypePtr> Type::get_element_type() const {
  if (!is_array()) {
    return std::nullopt;
  }

  return std::get<type::Array>(kind).element_type;
}

std::optional<TypePtr> Type::get_root_element_type() const {
  if (!is_array()) {
    return std::nullopt;
  }

  auto element_type = std::get<type::Array>(kind).element_type;
  while (element_type->is_array()) {
    element_type = element_type->get_element_type().value();
  }

  return element_type;
}

std::optional<TypePtr> Type::get_value_type() const {
  if (!is_pointer()) {
    return std::nullopt;
  }
  return std::get<type::Pointer>(kind).value_type;
}

std::optional<TypePtr> Type::get_ret_type() const {
  if (!is_function()) {
    return std::nullopt;
  }
  return std::get<type::Function>(kind).ret_type;
}

std::string Type::to_string() const {
  if (is_bool()) {
    return "BOOL";
  }
  if (is_int()) {
    return "INT";
  }
  if (is_float()) {
    return "FLOAT";
  }
  if (is_array()) {
    auto array = std::get<type::Array>(kind);
    std::string length = array.maybe_length.has_value()
                           ? std::to_string(array.maybe_length.value()) + " "
                           : "";
    return "[" + length + "x " + array.element_type->to_string() + "]";
  }
  if (is_void()) {
    return "VOID";
  }
  if (is_pointer()) {
    return get_value_type().value()->to_string() + "*";
  }
  if (is_function()) {
    auto function = std::get<type::Function>(kind);
    std::string params = "";
    for (auto param : function.param_types) {
      params += param->to_string() + ", ";
    }
    if (params.size() > 0) {
      params.pop_back();
      params.pop_back();
    }
    return function.ret_type->to_string() + "(" + params + ")";
  }
  // unreachable actually.
  return "UNREACHABLE_TYPE";
}

size_t Type::get_size() const {
  return std::visit(
    overloaded{
      [](const type::Integer& kind) -> size_t { return kind.size; },
      [](const type::Float& kind) -> size_t { return 32; },
      [](const type::Void& kind) -> size_t { return 0; },
      [](const type::Pointer& kind) -> size_t { return 64; },
      [](const type::Array& kind) -> size_t {
        if (!kind.maybe_length.has_value()) {
          return 64;
        } else {
          return kind.element_type->get_size() * kind.maybe_length.value();
        }
      },
      [](const auto&) -> size_t { return 0; },
    },
    this->kind
  );
}

TypePtr create_int_type() {
  return std::make_shared<Type>(TypeKind(type::Integer{32}));
}

TypePtr create_bool_type() {
  return std::make_shared<Type>(TypeKind(type::Integer{1}));
}

TypePtr create_float_type() {
  return std::make_shared<Type>(TypeKind(type::Float{}));
}

TypePtr create_void_type() {
  return std::make_shared<Type>(TypeKind(type::Void{}));
}

TypePtr
create_array_type(TypePtr element_type, std::optional<size_t> maybe_length) {
  return std::make_shared<Type>(TypeKind(type::Array{element_type, maybe_length}
  ));
}

TypePtr create_pointer_type(TypePtr value_type) {
  return std::make_shared<Type>(TypeKind(type::Pointer{value_type}));
}

TypePtr
create_function_type(TypePtr ret_type, std::vector<TypePtr> param_types) {
  return std::make_shared<Type>(TypeKind(type::Function{ret_type, param_types})
  );
}

std::optional<TypePtr> create_array_type_from_expr(
  TypePtr element_type,
  std::optional<ast::ExprPtr> maybe_expr
) {
  if (!maybe_expr.has_value()) {
    return std::make_optional(create_array_type(element_type, std::nullopt));
  }

  auto expr = maybe_expr.value();
  if (!expr->is_comptime()) {
    return std::nullopt;
  }
  auto comptime_count = expr->get_comptime_value();
  if (!comptime_count.has_value()) {
    return std::nullopt;
  }

  auto comptime_count_int =
    comptime_compute_cast(comptime_count.value(), create_int_type());

  int count = std::get<int>(comptime_count_int->kind);
  auto type =
    create_array_type(element_type, std::make_optional((size_t)count));

  return std::make_optional(type);
}

bool operator==(TypePtr lhs, TypePtr rhs) {
  if (lhs->kind.index() != rhs->kind.index()) {
    return false;
  }

  return std::visit(
    overloaded{
      [](type::Integer& lhs, type::Integer& rhs) {
        return lhs.size == rhs.size;
      },
      [](type::Float& lhs, type::Float& rhs) { return true; },
      [](type::Array& lhs, type::Array& rhs) {
        if (lhs.maybe_length.has_value() != rhs.maybe_length.has_value()) {
          return false;
        }
        if (lhs.maybe_length.has_value()) {
          if (lhs.maybe_length.value() != rhs.maybe_length.value()) {
            return false;
          }
        }
        return lhs.element_type == rhs.element_type;
      },
      [](type::Void& lhs, type::Void& rhs) { return true; },
      [](type::Pointer& lhs, type::Pointer& rhs) {
        return lhs.value_type == rhs.value_type;
      },
      [](auto& lhs, auto& rhs) { return false; },
    },
    lhs->kind, rhs->kind
  );
}

}  // namespace frontend

}  // namespace syc