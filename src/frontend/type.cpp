#include "type.h"

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

TypePtr Type::get_element_type() const {
  if (!is_array()) {
    return nullptr;
  }

  return std::get<type::Array>(kind).element_type;
}

TypePtr Type::get_root_element_type() const {
  if (!is_array()) {
    return nullptr;
  }

  TypePtr element_type = std::get<type::Array>(kind).element_type;

  while (element_type->is_array()) {
    element_type = element_type->get_element_type();
  }

  return element_type;
}

TypePtr Type::get_value_type() const {
  if (is_pointer()) {
    return std::get<type::Pointer>(kind).value_type;
  }
  return nullptr;
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
create_array_type(TypePtr element_type, std::optional<size_t> maybe_size) {
  return std::make_shared<Type>(TypeKind(type::Array{element_type, maybe_size})
  );
}

TypePtr create_pointer_type(TypePtr value_type) {
  return std::make_shared<Type>(TypeKind(type::Pointer{value_type}));
}

TypePtr
create_function_type(TypePtr ret_type, std::vector<TypePtr> param_types) {
  return std::make_shared<Type>(TypeKind(type::Function{ret_type, param_types})
  );
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
        if (lhs.maybe_size.has_value() != rhs.maybe_size.has_value()) {
          return false;
        }
        if (lhs.maybe_size.has_value()) {
          if (lhs.maybe_size.value() != rhs.maybe_size.value()) {
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