#ifndef SYC_PASSES_ASM_STORE_FUSE_H_
#define SYC_PASSES_ASM_STORE_FUSE_H_

#include "common.h"

namespace syc {
namespace backend {

void store_fuse(Builder& builder);

void store_fuse_function(FunctionPtr function, Builder& builder);

void store_fuse_simple_basic_block(BasicBlockPtr basic_block, Builder& builder);

void store_fuse_compl_basic_block(BasicBlockPtr basic_block, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif  // SYC_PASSES_ASM_STORE_FUSE_H_