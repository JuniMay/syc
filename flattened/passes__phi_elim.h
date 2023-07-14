#ifndef SYC_PASSES_PHI_ELIM_H_
#define SYC_PASSES_PHI_ELIM_H_

#include "common.h"
#include "backend__builder.h"
#include "backend__function.h"

namespace syc {
namespace backend {

void phi_elim(Builder& builder);

void phi_elim_function(FunctionPtr function, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif