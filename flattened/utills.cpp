#include "utils.h"

namespace syc {

std::string indent_str(const std::string &text, const std::string &indent) {
  std::string result = "";

  std::istringstream iss(text);
  std::string line;

  while (std::getline(iss, line)) {
    result += indent + line + "\n";
  }
  if (text.length() > 0) {
    result.pop_back();
  }
  
  return result;
}

}