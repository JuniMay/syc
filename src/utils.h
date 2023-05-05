#ifndef SYC_UTILS_H_
#define SYC_UTILS_H_

#include "common.h"
#include <filesystem>

namespace syc {

/// Indent each line of `text` by adding `indent` to the start.
std::string indent_str(const std::string& text, const std::string& indent);

/// If a string starts with the given prefix.
bool starts_with(const std::string& str, const std::string& prefix);

/// Options from the commandline arguments.
struct Options {
  std::string input_filename;
  int optimization_level;
  std::optional<std::string> output_file;
  std::optional<std::string> token_file;
  std::optional<std::string> ast_file;
  std::optional<std::string> ir_file;
};

Options parse_args(int argc, char** argv);

}  // namespace syc

#endif