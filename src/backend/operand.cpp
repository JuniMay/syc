#include "backend/operand.h"

namespace syc {
namespace backend {

Operand::Operand(OperandID id, OperandKind kind, Modifier modifier)
  : id(id), kind(kind), modifier(modifier) {}

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
  auto it = std::find(use_id_list.begin(), use_id_list.end(), use_id);
  if (it != use_id_list.end()) {
    use_id_list.erase(it);
  }
}

std::string Operand::to_string(int width) const {
  std::string result = "";

  if (modifier == Modifier::Lo) {
    result += "\%lo(";
  } else if (modifier == Modifier::Hi) {
    result += "\%hi(";
  }

  std::visit(
    overloaded{
      [&result, width](const Immediate& immediate) {
        result += immediate.to_string(width);
      },
      [&result](const Register& reg) { result += reg.to_string(); },
      [&result](const VirtualRegister& vreg) { result += vreg.to_string(); },
      [&result](const Global& global) { result += global.name; },
      [&result](const LocalMemory& local) {
        result += std::to_string(local.offset);
      },
    },
    kind
  );

  if (modifier != Modifier::None) {
    result += ")";
  }

  return result;
}

bool Operand::is_local_memory() const {
  return std::holds_alternative<LocalMemory>(kind);
}

bool Operand::is_immediate() const {
  return std::holds_alternative<Immediate>(kind);
}

bool Operand::is_vreg() const {
  return std::holds_alternative<VirtualRegister>(kind);
}

bool Operand::is_reg() const {
  return std::holds_alternative<Register>(kind);
}

bool Operand::is_global() const {
  return std::holds_alternative<Global>(kind);
}

bool Operand::is_float() const {
  if (is_reg()) {
    return std::get<Register>(kind).is_float();
  } else if (is_vreg()) {
    return std::get<VirtualRegister>(kind).is_float();
  } else {
    return false;
  }
}

bool Operand::operator==(const Operand& rhs) const {
  if (this->is_vreg() && rhs.is_vreg()) {
    return this->id == rhs.id;
  } else if (this->is_reg() && rhs.is_reg()) {
    const auto& lhs_reg = std::get<Register>(this->kind);
    const auto& rhs_reg = std::get<Register>(rhs.kind);

    if (lhs_reg.reg.index() != rhs_reg.reg.index()) {
      return false;
    }

    if (lhs_reg.is_general()) {
      return std::get<GeneralRegister>(lhs_reg.reg) ==
             std::get<GeneralRegister>(rhs_reg.reg);
    } else {
      return std::get<FloatRegister>(lhs_reg.reg) ==
             std::get<FloatRegister>(rhs_reg.reg);
    }
  } else {
    return false;
  }
}

bool Operand::is_zero() const {
  if (is_immediate()) {
    return std::get<Immediate>(kind).is_zero();
  } else if (is_reg()) {
    auto reg = std::get<Register>(kind);

    if (reg.is_general()) {
      return std::get<GeneralRegister>(reg.reg) == GeneralRegister::Zero;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool Operand::is_sp() const {
  if (is_reg()) {
    auto reg = std::get<Register>(kind);

    if (reg.is_general()) {
      return std::get<GeneralRegister>(reg.reg) == GeneralRegister::Sp;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

}  // namespace backend
}  // namespace syc