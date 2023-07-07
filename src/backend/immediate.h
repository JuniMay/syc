#ifndef SYC_BACKEND_IMMEDIATE_H_
#define SYC_BACKEND_IMMEDIATE_H_

#include "common.h"

namespace syc {
namespace backend {

/// Immediate in the assembly code.
/// Value check is not performed here. The value shall be converted into the
/// right form in the construction of the assembly code.
struct Immediate {
  std::variant<int32_t, int64_t, uint32_t, uint64_t> value;

  std::string to_string(int width = 0) const;
};

}  // namespace backend

}  // namespace syc

#endif