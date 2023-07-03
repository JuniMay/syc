#ifndef SYC_BACKEND_GLOBAL_H_
#define SYC_BACKEND_GLOBAL_H_

#include "common.h"

namespace syc {
namespace backend {

struct Global {
  std::variant<int32_t, int64_t, uint32_t, uint64_t> value;
  std::string name;

  std::string value_string() const;

  size_t get_size() const;
};

}  // namespace backend

}  // namespace syc

#endif // GLOBAL_H
