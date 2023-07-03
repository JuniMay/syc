#include "backend/builder.h"
#include "backend/global.h"
#include "backend/immediate.h"

namespace syc {
namespace backend {

OperandID Builder::fetch_operand(OperandKind kind, Modifier modifier) {
  auto id = context.get_next_operand_id();
  auto operand = std::make_shared<Operand>(id, kind, modifier);
  context.register_operand(operand);
  return id;
}

OperandID Builder::fetch_immediate(
  std::variant<int32_t, int64_t, uint32_t, uint64_t> value
) {
  auto kind = Immediate{value};
  return fetch_operand(kind, Modifier::None);
}

OperandID Builder::fetch_global(
  std::string name,
  std::variant<std::vector<uint32_t>, uint64_t> value
) {
  auto kind = Global{value, name};
  return fetch_operand(kind, Modifier::None);
}

OperandID Builder::fetch_virtual_register(VirtualRegisterKind kind) {
  auto vreg_id = context.get_next_virtual_register_id();
  auto vreg = VirtualRegister{vreg_id, kind};
  return fetch_operand(vreg, Modifier::None);
}

OperandID Builder::fetch_register(Register reg) {
  return fetch_operand(reg, Modifier::None);
}

OperandID Builder::fetch_local_memory(size_t offset) {
  auto kind = LocalMemory {offset};
  return fetch_operand(kind, Modifier::None);
}

}  // namespace backend
}  // namespace syc