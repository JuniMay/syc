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

}  // namespace backend
}  // namespace syc