#ifndef SYC_PASSES_IR_STRAIGHTEN_H_
#define SYC_PASSES_IR_STRAIGHTEN_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__basic_block.h"

namespace syc {
namespace ir {

void straighten(Builder& builder);

void straighten_function(FunctionPtr function, Builder& builder);

}
}

#endif