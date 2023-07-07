#include "backend/global.h"

namespace syc {
namespace backend {

size_t Global::get_size() const {
  return std::visit(
    overloaded{
      [](const std::vector<uint32_t>& v) -> size_t { return 4 * v.size(); },
      [](uint64_t v) -> size_t { return v; },
    },
    value
  );
}

std::string Global::to_string() const {
  std::string result = "\t.type " + name + ", @object\n";

  std::visit(
    overloaded{
      [&result, this](const std::vector<uint32_t>& v) {
        result += "\t.data\n";
        result += "\t.globl " + name + "\n";
        result += "\t.align 2\n";
        result += name + ":\n";
        for (auto& i : v) {
          result += "\t.word " + std::to_string(i) + "\n";
        }
      },
      [&result, this](uint64_t v) {
        result += "\t.bss\n";
        result += "\t.globl " + name + "\n";
        result += "\t.align 2\n";
        result += name + ":\n";
        result += "\t.zero " + std::to_string(v) + "\n";
      },
    },
    value
  );
  result += "\t.size " + name + ", " + std::to_string(get_size()) + "\n";

  return result;
}

}  // namespace backend

}  // namespace syc