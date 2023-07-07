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

std::string to_string(const Type& type);
std::string to_string(TypePtr type);

}  // namespace type

bool operator==(const Type& lhs, const Type& rhs);
bool operator==(TypePtr lhs, TypePtr rhs);

bool operator!=(const Type& lhs, const Type& rhs);
bool operator!=(TypePtr lhs, TypePtr rhs);

size_t get_size(const Type& type);
size_t get_size(TypePtr type);

}  // namespace ir
}  // namespace syc

#endif