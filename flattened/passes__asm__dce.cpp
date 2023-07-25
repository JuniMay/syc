#include "passes__asm__dce.h"
#include "backend__builder.h"
#include "backend__context.h"
#include "backend__function.h"
#include "backend__instruction.h"
#include "backend__operand.h"

namespace syc {
namespace backend {

void dce(Builder& builder) {
  // TODO: more elimination

  // remove all the operands and its corresponding def instructions
  for (auto [operand_id, operand] : builder.context.operand_table) {
    if (operand->is_vreg() && operand->use_id_list.size() == 0 && operand->def_id_list.size() == 1) {
      auto instruction =
        builder.context.get_instruction(operand->def_id_list[0]);
      instruction->remove(builder.context);
    }
  }
}

}  // namespace backend
}  // namespace syc