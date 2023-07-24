#include <sstream>

#include "common.h"
#include "ir/type.h"

namespace syc {
namespace ir {

std::string Type::to_string() const {
  using namespace type;
  std::stringstream ss;
  std::visit(
    overloaded{
      [&ss](const Void& a) { ss << "void"; },
      [&ss](const Integer& a) { ss << "i" << a.size; },
      [&ss](const Float& a) { ss << "float"; },
      [&ss](const Array& a) {
        ss << "[" << a.length << " x " << a.element_type->to_string() << "]";
      },
      [&ss](const Pointer& a) { ss << a.value_type->to_string() << "*"; }},
    kind
  );
  return ss.str();
}

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
    lhs.kind, rhs.kind
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

size_t Type::get_size() const {
  using namespace type;
  return std::visit(
    overloaded{
      [](const Void& a) -> size_t { return 0; },
      [](const Integer& a) -> size_t { return a.size; },
      [](const Float& a) -> size_t { return 32; },
      [](const Array& a) -> size_t {
        return a.length * a.element_type->get_size();
      },
      [](const Pointer& a) -> size_t { return 64; },
    },
    kind
  );
}

size_t get_size(TypePtr type) {
  return type->get_size();
}

}  // namespace ir
}  // namespace syc