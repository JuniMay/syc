#include "backend__immediate.h"

namespace syc {
namespace backend {

std::string Immediate::to_string() const {
  return std::visit(
    overloaded{
      [](int32_t v) { return std::to_string(v); },
      [](int64_t v) { return std::to_string(v); },
      [](uint32_t v) {
        std::stringstream ss;
        ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << v;
        return ss.str();
      },
      [](uint64_t v) {
        std::stringstream ss;
        ss << "0x" << std::setfill('0') << std::setw(16) << std::hex << v;
        return ss.str();
      },
    },
    value
  );
}

}  // namespace backend
}  // namespace syc