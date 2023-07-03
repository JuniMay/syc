#include "backend/operand.h"

namespace syc {
namespace backend {

void Operand::set_def(InstructionID def_id) {
  this->def_id = def_id;
}

void Operand::add_use(InstructionID use_id) {
  use_id_list.push_back(use_id);
}

std::string Operand::to_string() const {
  return std::visit(
    overloaded{
      [](const Immediate& immediate) { return immediate.to_string(); },
      [](const Register& reg) { return reg.to_string(); },
      [](const VirtualRegister& vreg) { return vreg.to_string(); },
      [](const Global& global) { return global.name; },
      [](const LocalMemory& local) { return std::to_string(local.offset); },
    },
    kind
  );
}

}  // namespace backend
}  // namespace syc