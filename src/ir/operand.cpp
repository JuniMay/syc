#include "ir/operand.h"
#include <sstream>
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {

namespace ir {

std::string operand::Constant::to_string(bool with_type) const {
  std::stringstream buf;

  if (with_type) {
    buf << this->type->to_string() << " ";
  }

  std::visit(
    overloaded{
      [&](int v) { buf << v; },
      [&](float v) {
        double v_double = static_cast<double>(v);
        uint64_t v_hexadecimal = *reinterpret_cast<uint64_t*>(&v_double);
        buf << "0x" << std::hex << std::uppercase << v_hexadecimal;
      },
      [&](Zeroinitializer v) { buf << "zeroinitializer"; },
      [&](const std::vector<ConstantPtr>& v) {
        buf << "[";
        for (size_t i = 0; i < v.size(); i++) {
          buf << v[i]->to_string(true);
          if (i != v.size() - 1) {
            buf << ", ";
          }
        }
        buf << "]";
      },
    },
    this->kind
  );

  return buf.str();
}

bool operand::Constant::is_zero() const {
  return std::visit(
    overloaded{
      [](int const_int) { return const_int == 0; },
      [](float const_float) { return const_float == 0.0; },
      [](auto const_zero) { return false; }},
    kind
  );
}

bool operand::Constant::get_bool_value() const {
  return std::visit(
    overloaded{
      [](int const_int) { return const_int != 0; },
      [](float const_float) { return const_float != 0.0; },
      [](ir::operand::ConstantPtr const_ptr) {
        return const_ptr->get_bool_value();
      },
      [](auto const_zero) { return false; }},
    kind
  );
}

Operand::Operand(OperandID id, TypePtr type, OperandKind kind)
  : id(id), type(type), kind(kind), maybe_def_id(std::nullopt) {}

std::string Operand::to_string(bool with_type) const {
  using namespace operand;
  return std::visit(
    overloaded{
      [](const Global& k) { return "@" + k.name; },
      [=](ConstantPtr k) { return k->to_string(with_type); },
      [](const Parameter& k) { return "%" + k.name; },
      [this](const Arbitrary& k) { return "%t" + std::to_string(id); },
    },
    kind
  );
}

TypePtr Operand::get_type() const {
  using namespace operand;
  return std::visit(
    overloaded{
      [this](const Global& k) {
        return std::make_shared<Type>(type::Pointer{});
      },
      [this](const auto& k) { return type; }},
    kind
  );
}
void Operand::set_def(InstructionID def_id) {
  this->maybe_def_id = def_id;
}

void Operand::add_use(InstructionID use_id) {
  use_id_list.push_back(use_id);
}

void Operand::remove_def() {
  this->maybe_def_id = std::nullopt;
}

void Operand::remove_use(InstructionID use_id) {
  this->use_id_list.erase(
    std::remove(this->use_id_list.begin(), this->use_id_list.end(), use_id),
    this->use_id_list.end()
  );
}

bool Operand::is_constant() const {
  return std::holds_alternative<operand::ConstantPtr>(kind);
}

bool Operand::is_int() const {
  return this->get_type()->as<type::Integer>().has_value();
}

bool Operand::is_parameter() const {
  return std::holds_alternative<operand::Parameter>(kind);
}

bool Operand::is_arbitrary() const {
  return std::holds_alternative<operand::Arbitrary>(kind);
}

bool Operand::is_zero() const {
  if (this->is_constant()) {
    return std::get<operand::ConstantPtr>(kind)->is_zero();
  }
  return false;
}

bool Operand::is_global() const {
  return std::holds_alternative<operand::Global>(kind);
}

}  // namespace ir

}  // namespace syc