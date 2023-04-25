#ifndef SYC_FRONTEND_TYPE_H_
#define SYC_FRONTEND_TYPE_H_

#include "common.h"

namespace syc {

namespace frontend {

namespace type {

struct Int {
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
  bool is_const;
};

}  // namespace frontend
}  // namespace syc

#endif