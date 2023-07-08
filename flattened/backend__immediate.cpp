#include "backend__immediate.h"

namespace syc {
namespace backend {

std::string Immediate::to_string(int width) const {
  return std::visit(
    overloaded{
      [](int32_t v) { return std::to_string(v); },
      [](int64_t v) { return std::to_string(v); },
      [=](uint32_t v) {
        std::stringstream ss;
        int w = (width == 0) ? 8 : width;
        ss << "0x" << std::setfill('0') << std::setw(w) << std::hex << v;
        return ss.str();
      },
      [=](uint64_t v) {
        std::stringstream ss;
        int w = (width == 0) ? 16 : width;
        ss << "0x" << std::setfill('0') << std::setw(w) << std::hex << v;
        return ss.str();
      },
    },
    value
  );
}

bool Immediate::is_zero() const {
  return std::visit(
    overloaded{
      [](int32_t v) { return v == 0; },
      [](int64_t v) { return v == 0; },
      [](uint32_t v) { return v == 0; },
      [](uint64_t v) { return v == 0; },
    },
    value
  );
}

}  // namespace backend
}  // namespace syc