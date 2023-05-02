#include <sstream>

#include "common.h"
#include "ir__type.h"

namespace syc {
namespace ir {

namespace type {

std::string to_string(const Type& type) {
  using namespace type;
  std::stringstream ss;
  std::visit(
    overloaded{
      [&ss](const Void& a) { ss << "void"; },
      [&ss](const Integer& a) { ss << "i" << a.size; },
      [&ss](const Float& a) { ss << "float"; },
      [&ss](const Array& a) {
        ss << "[" << a.length << " x " << to_string(*a.element_type) << "]";
      },
      [&ss](const Pointer& a) { ss << to_string(*a.value_type) << "*"; }},
    type
  );
  return ss.str();
}

std::string to_string(TypePtr type) {
  return to_string(*type);
}

}  // namespace type

bool operator==(const Type& lhs, const Type& rhs) {
  using namespace type;
  return std::visit(
    overloaded{
      [](const Void& a, const Void& b) { return true; },
      [](const Integer& a, const Integer& b) { return a.size == b.size; },
      [](const Float& a, const Float& b) { return true; },
      [](const Array& a, const Array& b) {
        return a.length == b.length && *a.element_type == *b.element_type;
      },
      [](const Pointer& a, const Pointer& b) {
        return *a.value_type == *b.value_type;
      },
      [](const auto& a, const auto& b) { return false; },
    },
    lhs, rhs
  );
}

bool operator==(TypePtr lhs, TypePtr rhs) {
  return *lhs == *rhs;
}

bool operator!=(const Type& lhs, const Type& rhs) {
  return !(lhs == rhs);
}

bool operator!=(TypePtr lhs, TypePtr rhs) {
  return !(lhs == rhs);
}

size_t get_size(const Type& type) {
  using namespace type;
  return std::visit(
    overloaded{
      [](const Void& a) -> size_t { return 0; },
      [](const Integer& a) -> size_t { return a.size; },
      [](const Float& a) -> size_t { return 32; },
      [](const Array& a) -> size_t {
        return a.length * get_size(*a.element_type);
      },
      [](const Pointer& a) -> size_t { return 64; }},
    type
  );
}

size_t get_size(TypePtr type) {
  return get_size(*type);
}

}  // namespace ir
}  // namespace syc