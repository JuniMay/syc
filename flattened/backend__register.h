#ifndef SYC_BACKEND_REGISTER_H_
#define SYC_BACKEND_REGISTER_H_

#include "common.h"

namespace syc {
namespace backend {

/// RISC-V general purpose registers.
enum class GeneralRegister {
  Zero = 0,
  Ra = 1,
  Sp = 2,
  Gp = 3,
  Tp = 4,
  T0 = 5,
  T1 = 6,
  T2 = 7,
  S0 = 8,
  S1 = 9,
  A0 = 10,
  A1 = 11,
  A2 = 12,
  A3 = 13,
  A4 = 14,
  A5 = 15,
  A6 = 16,
  A7 = 17,
  S2 = 18,
  S3 = 19,
  S4 = 20,
  S5 = 21,
  S6 = 22,
  S7 = 23,
  S8 = 24,
  S9 = 25,
  S10 = 26,
  S11 = 27,
  T3 = 28,
  T4 = 29,
  T5 = 30,
  T6 = 31,
};

/// RISC-V floating point registers.
enum class FloatRegister {
  Ft0 = 0,
  Ft1 = 1,
  Ft2 = 2,
  Ft3 = 3,
  Ft4 = 4,
  Ft5 = 5,
  Ft6 = 6,
  Ft7 = 7,
  Fs0 = 8,
  Fs1 = 9,
  Fa0 = 10,
  Fa1 = 11,
  Fa2 = 12,
  Fa3 = 13,
  Fa4 = 14,
  Fa5 = 15,
  Fa6 = 16,
  Fa7 = 17,
  Fs2 = 18,
  Fs3 = 19,
  Fs4 = 20,
  Fs5 = 21,
  Fs6 = 22,
  Fs7 = 23,
  Fs8 = 24,
  Fs9 = 25,
  Fs10 = 26,
  Fs11 = 27,
  Ft8 = 28,
  Ft9 = 29,
  Ft10 = 30,
  Ft11 = 31,
};

struct Register {
  std::variant<GeneralRegister, FloatRegister> reg;

  bool is_general() const { return std::holds_alternative<GeneralRegister>(reg); }
  bool is_float() const { return std::holds_alternative<FloatRegister>(reg); }

  std::string to_string() const;
};

/// Virtual register kind.
enum class VirtualRegisterKind {
  /// General purpose register.
  General,
  /// Floating point register.
  Float,
};

/// Virtual register.
struct VirtualRegister {
  VirtualRegisterID id;
  VirtualRegisterKind kind;

  bool is_general() const { return kind == VirtualRegisterKind::General; }
  bool is_float() const { return kind == VirtualRegisterKind::Float; }

  std::string to_string() const;
};

}  // namespace backend
}  // namespace syc

#endif