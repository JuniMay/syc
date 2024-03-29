#ifndef SYC_FRONTEND_TYPE_H_
#define SYC_FRONTEND_TYPE_H_

#include "common.h"
#include "frontend/comptime.h"

namespace syc {

namespace frontend {

namespace type {

/// Integer-like type.
/// This is for bool and int.
struct Integer {
  /// Size of the type in bits.
  /// 1 for bool, 32 for int.
  /// Actually bool is store as a byte in machine code, but here just treat it
  /// as 1 bit.
  size_t size;
};

struct Float {};

struct Array {
  TypePtr element_type;
  size_t length;
};

struct Void {};

struct Pointer {
  TypePtr value_type;
};

struct Function {
  TypePtr ret_type;
  std::vector<TypePtr> param_types;
};

}  // namespace type

struct Type {
  TypeKind kind;

  Type(TypeKind kind);

  bool is_bool() const;
  /// If the type is `int`.
  /// Note that `bool` is not `int`.
  bool is_int() const;
  bool is_float() const;
  bool is_array() const;
  bool is_void() const;
  bool is_pointer() const;
  bool is_function() const;

  /// Get the element type of an array type.
  /// If the type is not an array, return `std::nullopt`.
  std::optional<TypePtr> get_element_type() const;

  /// Get the most elementary type of an array type.
  /// If the type is not an array, return `std::nullopt`.
  std::optional<TypePtr> get_root_element_type() const;

  /// Get the value type of a pointer type.
  /// If the type is not a pointer, return `std::nullopt`.
  std::optional<TypePtr> get_value_type() const;

  std::optional<TypePtr> get_ret_type() const;

  /// Convert the type to string.
  std::string to_string() const;

  size_t get_size() const;
};

TypePtr create_int_type();
TypePtr create_bool_type();
TypePtr create_float_type();
TypePtr create_void_type();
TypePtr
create_array_type(TypePtr element_type, std::optional<size_t> maybe_length);
TypePtr create_pointer_type(TypePtr value_type);
TypePtr
create_function_type(TypePtr ret_type, std::vector<TypePtr> param_types);

/// Create an array type from ast expression (optional).
std::optional<TypePtr> create_array_type_from_expr(
  TypePtr element_type,
  std::optional<ast::ExprPtr> maybe_expr
);

bool operator==(TypePtr lhs, TypePtr rhs);

}  // namespace frontend
}  // namespace syc

#endif