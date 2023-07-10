#ifndef SYC_PASSES_MEM2REG_H_
#define SYC_PASSES_MEM2REG_H_

#include "common.h"
#include "ir/builder.h"
#include "ir/function.h"

namespace syc {
namespace ir {

void mem2reg(Builder& builder);

void mem2reg_function(FunctionPtr function, Builder& builder);
  
}
}

#endif