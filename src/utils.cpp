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

  std::map<std::string, std::string> arguments;
  std::vector<std::string> positional_arguments;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg[0] == '-') {
      std::string key = arg.substr(1);
      if (key[0] == '-')
        key = key.substr(1);
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        arguments[key] = argv[++i];
      } else {
        arguments[key] = "";
      }
    } else {
      positional_arguments.push_back(arg);
    }
  }

  // input file
  if (positional_arguments.size() > 1) {
    std::cerr << "error: too many input files" << std::endl;
    exit(1);
  } else if (positional_arguments.size() < 1 || positional_arguments[0] == "") {
    std::cerr << "error: no input file specified" << std::endl;
    exit(1);
  } else {
    options.input_filename = positional_arguments[0];
  }
  if (!std::filesystem::exists(options.input_filename)) {
    std::cerr << "error: input file not exists" << std::endl;
    exit(1);
  }

  // output file
  if (arguments.find("o") != arguments.end()) {
    options.output_file = arguments.find("o")->second;
    if (options.output_file == "") {
      std::cerr << "error: no output file specified" << std::endl;
      exit(1);
    }
  } else {
    options.output_file = "a.asm";
  }

  // tokens file
  if (arguments.find("emit-tokens") != arguments.end()) {
    options.token_file = arguments.find("emit-tokens")->second;
    if (options.token_file == "") {
      std::cerr << "error: no tokens file specified" << std::endl;
      exit(1);
    }
  }

  // ast file
  if (arguments.find("emit-ast") != arguments.end()) {
    options.ast_file = arguments.find("emit-ast")->second;
    if (options.ast_file == "") {
      std::cerr << "error: no ast file specified" << std::endl;
      exit(1);
    }
  }

  // ir file
  if (arguments.find("emit-ir") != arguments.end()) {
    options.ir_file = arguments.find("emit-ir")->second;
    if (options.ir_file == "") {
      std::cerr << "error: no ir file specified" << std::endl;
      exit(1);
    }
  }

  // if aggresive
  if (arguments.find("aggressive") != arguments.end()) {
    options.aggressive_opt = true;
  } else {
    options.aggressive_opt = false;
  }

  // Optimization level
  if (arguments.find("O0") != arguments.end()) {
    options.optimization_level = 0;
  }
  if (arguments.find("O1") != arguments.end()) {
    options.optimization_level = 1;
  }
  if (arguments.find("O2") != arguments.end()) {
    options.optimization_level = 2;
  }
  if (arguments.find("O3") != arguments.end()) {
    options.optimization_level = 3;
  }

  // -S
  if (arguments.find("S") != arguments.end()) {
    // do nothing
  }
  return options;
}

}  // namespace syc