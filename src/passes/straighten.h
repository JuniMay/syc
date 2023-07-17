#ifndef SYC_PASSES_STRAIGHTEN_H_
#define SYC_PASSES_STRAIGHTEN_H_

#include "common.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/basic_block.h"

namespace syc {
namespace ir {

void straighten(Builder& builder);

void straighten_function(FunctionPtr function, Builder& builder);

}
}

#endif