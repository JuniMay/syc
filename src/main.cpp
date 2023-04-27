
#include <iostream>
#include "ir/builder.h"
#include "ir/instruction.h"
#include "frontend/driver.h"

#include "frontend/generated/lexer.h"

#ifdef UNITTEST
#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"
#include "unittest/backend.h"
#include "unittest/ir.h"
#endif

int main(int argc, char* argv[]) {
#ifdef UNITTEST
  int result = Catch::Session().run(argc, argv);
  return result;
#else

  using namespace syc;

  // TODO: Commandline arguments
  frontend::Driver parse_driver(argv[1]);

  int parse_success = parse_driver.parser->parse();

  auto compunit = &parse_driver.compunit;

  std::cout << parse_driver.tokens;

  if (parse_success != 0) {
    std::cerr << "Parse failed." << std::endl;
    return 1;
  }

  return 0;

#endif
}
