#ifndef SYC_PASSES_IR_STORE_ELIM_H_
#define SYC_PASSES_IR_STORE_ELIM_H_

#include "common.h"
#include "ir/builder.h"

namespace syc {
namespace ir {

void store_elim(Builder& builder);

}
}

#endif