#include "backend/operand.h"

namespace syc {
namespace backend {

void Operand::set_def(InstructionID def_id) {
  this->def_id = def_id;
}

void Operand::add_use(InstructionID use_id) {
  use_id_list.push_back(use_id);
}

}  // namespace backend
}  // namespace syc