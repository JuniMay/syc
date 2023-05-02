
#include <fstream>
#include <iostream>
#include "frontend__driver.h"
#include "frontend__irgen.h"
#include "ir__builder.h"
#include "ir__instruction.h"
#include "utils.h"

#ifdef UNITTEST
#define CATCH_CONFIG_RUNNER
#include "catch2__catch.hpp"
#include "unittest__backend.h"
#include "unittest__ir.h"
#endif

int main(int argc, char* argv[]) {
#ifdef UNITTEST
  int result = Catch::Session().run(argc, argv);
  return result;
#else

  using namespace syc;

  auto options = parse_args(argc, argv);

  frontend::Driver parse_driver(options.input_filename);

  int parse_success = parse_driver.parser->parse();

  auto& compunit = parse_driver.compunit;

  if (options.token_file.has_value()) {
    std::ofstream token_file(options.token_file.value());
    token_file << parse_driver.tokens;
  }

  if (options.ast_file.has_value()) {
    std::ofstream ast_file(options.ast_file.value());
    ast_file << compunit.to_string();
  }

  auto ir_builder = ir::Builder();

  irgen_compunit(compunit, ir_builder);

  if (options.ir_file.has_value()) {
    std::ofstream ir_file(options.ir_file.value());
    ir_file << ir_builder.context.to_string();
  }

  if (parse_success != 0) {
    std::cerr << "Parse failed." << std::endl;
    return 1;
  }

  return 0;

#endif
}
