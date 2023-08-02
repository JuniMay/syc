#ifndef SYC_PASSES_IR_PURITY_OPT_H_
#define SYC_PASSES_IR_PURITY_OPT_H_

#include "common.h"
#include "ir/builder.h"

namespace syc {
namespace ir {

struct PurityOptContext {
  std::unordered_map<std::string, bool> purity_result;
};

void purity_opt(Builder& builder);

bool is_pure(
  FunctionPtr function,
  Builder& builder,
  PurityOptContext& purity_ctx,
  size_t depth = 0
);

void purity_opt_function(
  FunctionPtr function,
  Builder& builder,
  PurityOptContext& purity_ctx
);

}  // namespace ir
}  // namespace syc

#endif