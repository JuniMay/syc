#ifndef SYC_BACKEND_OPERAND_H_
#define SYC_BACKEND_OPERAND_H_

#include "backend/immediate.h"
#include "backend/register.h"
#include "common.h"

namespace syc {
namespace backend {

struct Operand {
  OperandID id;
  OperandKind kind;

  std::optional<InstructionID> def_id;
  std::vector<InstructionID> use_id_list;

  void set_def(InstructionID def_id);

  void add_use(InstructionID use_id);

  std::string to_string() const;
};

}  // namespace backend
}  // namespace syc

#endif