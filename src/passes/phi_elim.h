#ifndef SYC_PASSES_PHI_ELIM_H_
#define SYC_PASSES_PHI_ELIM_H_

#include "common.h"
#include "ir/builder.h"
#include "ir/function.h"

namespace syc {
namespace ir {

void phi_elim(Builder& builder);

void phi_elim_function(FunctionPtr function, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif