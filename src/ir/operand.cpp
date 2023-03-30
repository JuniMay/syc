#include "ir/operand.h"
#include <sstream>
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
          overloaded{
            [](int v) { return std::to_string(v); },
            [](float v) {
              // convert float to double and hexadecimal integer
              // form to meet the requirement of llvm
              double v_double = static_cast<double>(v);
              uint64_t v_hexadecimal = *reinterpret_cast<uint64_t*>(&v_double);
              std::stringstream ss;
              ss << "0x" << std::hex << std::uppercase << v_hexadecimal;
              return ss.str();
            }},
          k.value
        );
      },
      [](const Parameter& k) { return "%" + k.name; },
      [this](const Arbitrary& k) { return "%t" + std::to_string(id); }},
    kind
  );
}

TypePtr Operand::get_type() {
  using namespace operand;
  return std::visit(
    overloaded{
      [this](const Global& k) {
        return std::make_shared<Type>(type::Pointer{type});
      },
      [this](const auto& k) { return type; }},
    kind
  );
}
void Operand::set_def(InstructionID def_id) {
  this->def_id = def_id;
}

void Operand::add_use(InstructionID use_id) {
  use_id_list.push_back(use_id);
}

}  // namespace ir

}  // namespace syc