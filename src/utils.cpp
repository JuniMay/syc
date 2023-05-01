#include "utils.h"

namespace syc {

std::string indent_str(const std::string& text, const std::string& indent) {
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

bool starts_with(const std::string& str, const std::string& prefix) {
  return str.compare(0, prefix.length(), prefix) == 0;
}

Options parse_args(int argc, char** argv) {
  Options options;
  options.optimization_level = 0;
  options.output_file = std::nullopt;
  options.token_file = std::nullopt;
  options.ast_file = std::nullopt;
  options.ir_file = std::nullopt;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-o") {
      if (i + 1 >= argc) {
        std::cerr << "error: no output file specified" << std::endl;
        exit(1);
      }
      options.output_file = argv[i + 1];
      i++;
    } else if (arg == "--emit-token") {
      if (i + 1 >= argc) {
        std::cerr << "error: no token file specified" << std::endl;
        exit(1);
      }
      options.token_file = argv[i + 1];
      i++;
    } else if (arg == "--emit-ast") {
      if (i + 1 >= argc) {
        std::cerr << "error: no ast file specified" << std::endl;
        exit(1);
      }
      options.ast_file = argv[i + 1];
      i++;
    } else if (arg == "--emit-ir") {
      if (i + 1 >= argc) {
        std::cerr << "error: no ir file specified" << std::endl;
        exit(1);
      }
      options.ir_file = argv[i + 1];
      i++;
    } else if (arg == "-O0") {
      options.optimization_level = 0;
    } else if (arg == "-O1") {
      options.optimization_level = 1;
    } else if (arg == "-O2") {
      options.optimization_level = 2;
    } else if (arg == "-O3") {
      options.optimization_level = 3;
    } else {
      options.input_filename = arg;
    }
  }
  return options;
}

}  // namespace syc