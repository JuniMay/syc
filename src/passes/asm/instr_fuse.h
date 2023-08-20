#ifndef SYC_PASSES_ASM_INSTR_FUSE_H_
#define SYC_PASSES_ASM_INSTR_FUSE_H_

#include "common.h"

namespace syc {
namespace backend {

void instr_fuse(Builder& builder);

void instr_fuse_function(FunctionPtr function, Builder& builder);

void instr_fuse_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace backend
}  // namespace syc


#endif 