#include "passes__asm_dce.h"
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
    if (operand->is_vreg() && operand->use_id_list.size() == 0 && operand->maybe_def_id.has_value()) {
      auto instruction =
        builder.context.get_instruction(operand->maybe_def_id.value());
      instruction->remove(builder.context);
    }
  }
}

}  // namespace backend
}  // namespace syc