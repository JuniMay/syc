#ifndef SYC_BACKEND_BUILDER_H_
#define SYC_BACKEND_BUILDER_H_

#include "backend/context.h"
#include "backend/operand.h"
#include "backend/register.h"
#include "common.h"

namespace syc {
namespace backend {

struct Builder {
  /// Context of the assembly
  Context context;
  /// Current function
  FunctionPtr curr_function;
  /// Current basic block
  BasicBlockPtr curr_basic_block;

  OperandID fetch_operand(OperandKind kind, Modifier modifier);

  OperandID fetch_immediate(
    std::variant<int32_t, int64_t, uint32_t, uint64_t> value
  );

  OperandID fetch_global(
    std::string name,
    std::variant<std::vector<uint32_t>, uint64_t> value
  );

  OperandID fetch_virtual_register(VirtualRegisterKind kind);

  OperandID fetch_register(Register reg);

  OperandID fetch_local_memory(size_t offset);

  Builder() = default;
};

}  // namespace backend
}  // namespace syc

#endif