
#include <iostream>
#include "ir/builder.h"
#include "ir/instruction.h"

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
  return 0;

#endif
}
