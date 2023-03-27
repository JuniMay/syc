#include "ir/operand.h"
#include "ir/type.h"

namespace syc {

namespace ir {

std::string Operand::to_string() const {
  using namespace operand;
  return std::visit(
      overloaded{
          [](const Global& k) { return "@" + k.name; },
          [](const Immediate& k) {
            return std::visit(
                overloaded{[](int v) { return std::to_string(v); },
                           [](float v) { return std::to_string(v); }},
                k.value);
          },
          [](const Parameter& k) { return "%" + k.name; },
          [this](const Arbitrary& k) { return "%t" + std::to_string(id); }},
      kind);
}

TypePtr Operand::get_type() {
  using namespace operand;
  return std::visit(
      overloaded{[this](const Global& k) {
                   return std::make_shared<Type>(type::Pointer{type});
                 },
                 [this](const auto& k) { return type; }},
      kind);
}

}  // namespace ir

}  // namespace syc