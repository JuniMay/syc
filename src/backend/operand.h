#ifndef SYC_BACKEND_OPERAND_H_
#define SYC_BACKEND_OPERAND_H_

#include "backend/global.h"
#include "backend/immediate.h"
#include "backend/register.h"
#include "common.h"

namespace syc {
namespace backend {

/// Local memory place for variables.
struct LocalMemory {
  /// Offset from the stack pointer.
  /// If this is for parameter, then the offset is from the fp.
  /// About sp and fp, see this discussion:
  /// https://stackoverflow.com/questions/74650564/is-frame-pointer-necessary-for-riscv-assembly
  int offset;
};

enum class Modifier {
  None,
  Lo,
  Hi,
};

struct Operand {
  OperandID id;
  OperandKind kind;

  /// The modifier of the operand.
  /// Modifier can only be used for symbols (globals).
  Modifier modifier = Modifier::None;

  Operand(OperandID id, OperandKind kind, Modifier modifier);

  std::optional<InstructionID> maybe_def_id;
  std::vector<InstructionID> use_id_list;

  void set_def(InstructionID def_id);

  void add_use(InstructionID use_id);

  std::string to_string(int width = 0) const;

  bool is_local_memory() const;

  bool is_immediate() const;

  bool is_vreg() const;

  bool is_reg() const;

  bool is_global() const;

  bool is_float() const;
};

}  // namespace backend
}  // namespace syc

#endif