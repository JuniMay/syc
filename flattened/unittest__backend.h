#include "backend__immediate.h"
#include "backend__operand.h"
#include "catch2__catch.hpp"

TEST_CASE("Machine Immediate") {
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

TEST_CASE("Machine Operand") {
  using namespace syc::backend;

  SECTION("Operand to string") {
    auto i32_imm = Operand{0, Immediate{(int32_t)123}};
    REQUIRE(i32_imm.to_string() == "123");

    auto i64_imm = Operand{0, Immediate{(int64_t)12345678}};
    REQUIRE(i64_imm.to_string() == "12345678");

    auto u32_imm = Operand{0, Immediate{(uint32_t)123}};
    REQUIRE(u32_imm.to_string() == "0x0000007b");

    auto u64_imm = Operand{0, Immediate{(uint64_t)12345678}};
    REQUIRE(u64_imm.to_string() == "0x0000000000bc614e");

    std::string gpreg_strs[32] = {
      "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
      "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
      "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

    std::string fpreg_strs[32] = {
      "ft0", "ft1", "ft2",  "ft3",  "ft4", "ft5", "ft6",  "ft7",
      "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4",  "fa5",
      "fa6", "fa7", "fs2",  "fs3",  "fs4", "fs5", "fs6",  "fs7",
      "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"};

    for (int i = 0; i < 32; i++) {
      auto gpreg = Operand{0, Register{(GPRegister)i}};
      REQUIRE(gpreg.to_string() == gpreg_strs[i]);

      auto fpreg = Operand{0, Register{(FPRegister)i}};
      REQUIRE(fpreg.to_string() == fpreg_strs[i]);
    }

    auto vreg = Operand{
      0,
      VirtualRegister{
        0,
        VirtualRegisterKind::Gp,
      },
    };

    REQUIRE(vreg.to_string() == "v0");

    auto vreg2 = Operand{
      0,
      VirtualRegister{
        1,
        VirtualRegisterKind::Fp,
      },
    };

    REQUIRE(vreg2.to_string() == "fv1");
  }
}