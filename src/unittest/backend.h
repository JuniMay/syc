#include "backend/immediate.h"
#include "catch2/catch.hpp"

TEST_CASE("Backend Immediate") {
  using namespace syc::backend;

  SECTION("Immediate to string") {
    auto i32_imm = Immediate{(int32_t)123};
    REQUIRE(i32_imm.to_string() == "123");

    auto i64_imm = Immediate{(int64_t)12345678};
    REQUIRE(i64_imm.to_string() == "12345678");

    auto u32_imm = Immediate{(uint32_t)123};
    REQUIRE(u32_imm.to_string() == "0x0000007b");

    auto u64_imm = Immediate{(uint64_t)12345678};
    REQUIRE(u64_imm.to_string() == "0x0000000000bc614e");
  }
}