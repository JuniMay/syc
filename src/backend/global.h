#ifndef SYC_BACKEND_GLOBAL_H_
#define SYC_BACKEND_GLOBAL_H_

#include "common.h"

namespace syc {
namespace backend {

struct Global {
  /// The value of the global variable.
  /// list of data in words or the size of zero
  std::variant<std::vector<uint32_t>, uint64_t> value;

  /// The name of the global variable.
  std::string name;

  /// The size of the global variable.
  size_t get_size() const;

  /// Convert to declaration in the assembly code.
  std::string to_string() const;
};

}  // namespace backend

}  // namespace syc

#endif // GLOBAL_H
