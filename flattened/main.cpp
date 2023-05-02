
#include <iostream>
#include "ir__builder.h"
#include "ir__instruction.h"
#include "frontend__driver.h"

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

  // TODO: Commandline arguments
  frontend::Driver parse_driver(argv[1]);

  int parse_success = parse_driver.parser->parse();

  auto compunit = &parse_driver.compunit;

  // std::cout << parse_driver.tokens;
  std::cout << compunit->to_string() << std::endl;

  if (parse_success != 0) {
    std::cerr << "Parse failed." << std::endl;
    return 1;
  }

  return 0;

#endif
}
