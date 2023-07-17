#ifndef SYC_PASSES_UNUSED_ELIM_H_
#define SYC_PASSES_UNUSED_ELIM_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"

namespace syc {
namespace ir {

void unused_elim(Builder& builder);

void unused_elim_function(FunctionPtr function, Builder& builder);

void unused_elim_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace ir
}  // namespace syc

#endif  // SYC_PASSES_UNUSED_ELIM_H_