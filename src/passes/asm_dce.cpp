#include "passes/asm_dce.h"
#include "backend/builder.h"
#include "backend/context.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"

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