#include "backend/global.h"

namespace syc {
namespace backend {

std::string Global::value_string() const{
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

size_t Global::get_size() const
{
  return std::visit(
    overloaded{
      [](int32_t v) { return sizeof(v); },
      [](int64_t v) { return sizeof(v); },
      [](uint32_t v) { return sizeof(v); },
      [](uint64_t v) { return sizeof(v); },
    },
    value
  );
}

}  // namespace backend

}  // namespace syc