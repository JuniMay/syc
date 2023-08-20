#ifndef SYC_PASSES_ASM_ADDR_SIMPLIFICATION_H_
#define SYC_PASSES_ASM_ADDR_SIMPLIFICATION_H_

#include "common.h"

namespace syc {
namespace backend {

void addr_simplification(Builder& builder);

void addr_simplification_function(FunctionPtr function, Builder& builder);

void addr_simplification_basic_block(BasicBlockPtr basic_block, Builder& builder);

std::variant<int32_t, int64_t, uint32_t, uint64_t> add_immediate(
  std::variant<int32_t, int64_t, uint32_t, uint64_t> a,
  std::variant<int32_t, int64_t, uint32_t, uint64_t> b
);

}  // namespace backend
}  // namespace syc

#endif