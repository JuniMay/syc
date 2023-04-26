#ifndef SYC_FRONTEND_TYPE_H_
#define SYC_FRONTEND_TYPE_H_

#include "common.h"

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
  size_t size;
  TypePtr element_type;
};

struct Void {};

struct Pointer {
  TypePtr value_type;
};

}  // namespace type

struct Type {
  TypeKind kind;

  bool is_bool() const;
  /// If the type is `int`.
  /// Note that `bool` is not `int`.
  bool is_int() const;
  bool is_float() const;
  bool is_array() const;
  bool is_void() const;
  bool is_pointer() const;

  /// Get the element type of an array type.
  /// Returns nullptr if the type is not an array.
  TypePtr get_element_type() const;

  /// Get the most elementary type of an array type.
  /// Returns nullptr if the type is not an array.
  TypePtr get_root_element_type() const;

  /// Get the value type of a pointer type.
  /// Returns nullptr if the type is not a pointer.
  TypePtr get_value_type() const;
};

TypePtr create_int_type();
TypePtr create_bool_type();
TypePtr create_float_type();
TypePtr create_void_type();
TypePtr create_array_type(size_t size, TypePtr element_type);
TypePtr create_pointer_type(TypePtr value_type);

bool operator==(TypePtr lhs, TypePtr rhs);

}  // namespace frontend
}  // namespace syc

#endif