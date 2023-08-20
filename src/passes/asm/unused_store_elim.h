#ifndef SYC_PASSES_ASM_UNUSED_STORE_ELIM_H_
#define SYC_PASSES_ASM_UNUSED_STORE_ELIM_H_

#include "common.h"

namespace syc {
namespace backend {

void unused_store_elim(Builder& builder);

void unused_store_elim_function(FunctionPtr function, Builder& builder);


}  // namespace backend
}  // namespace syc

#endif // SYC_PASSES_ASM_UNUSED_STORE_ELIM_H_