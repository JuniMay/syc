#ifndef SYC_IR_TYPE_H_
#define SYC_IR_TYPE_H_

#include "common.h"

namespace syc {
namespace ir {
namespace type {

struct Void {};

struct Integer {
  size_t size;
};

struct Float {};

struct Array {
  size_t length;
  TypePtr element_type;
};

struct Pointer {
  TypePtr value_type;
};

}  // namespace type

struct Type {
  TypeKind kind;

  Type(TypeKind kind) : kind(kind){};

  std::string to_string() const;
  size_t get_size() const;

  template <typename T>
  std::optional<T> as();
};

bool operator==(const Type& lhs, const Type& rhs);
bool operator==(TypePtr lhs, TypePtr rhs);

bool operator!=(const Type& lhs, const Type& rhs);
bool operator!=(TypePtr lhs, TypePtr rhs);

size_t get_size(TypePtr type);

template <typename T>
std::optional<T> Type::as() {
  if (!std::holds_alternative<T>(this->kind)) {
    return std::nullopt;
  } else {
    return std::get<T>(this->kind);
  }
}

}  // namespace ir
}  // namespace syc

#endif