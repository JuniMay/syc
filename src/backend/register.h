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

  bool is_general() const {
    return std::holds_alternative<GeneralRegister>(reg);
  }
  bool is_float() const { return std::holds_alternative<FloatRegister>(reg); }

  std::string to_string() const;
};

/// Hash for Register.
struct RegisterHash {
  std::size_t operator()(const Register& reg) const {
    if (reg.is_general()) {
      return (size_t)std::get<GeneralRegister>(reg.reg);
    } else {
      return (size_t)std::get<FloatRegister>(reg.reg) + 32;
    }
  }
};

bool operator==(const Register& lhs, const Register& rhs);
bool operator<(const Register& lhs, const Register& rhs);

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

#define PHYS_REG_GENERAL(X) (Register{GeneralRegister::X})
#define PHYS_REG_FLOAT(X) (Register{FloatRegister::X})

const std::vector<Register> REG_ALLOC = {
  PHYS_REG_GENERAL(A0),  PHYS_REG_GENERAL(A1),  PHYS_REG_GENERAL(A2),
  PHYS_REG_GENERAL(A3),  PHYS_REG_GENERAL(A4),  PHYS_REG_GENERAL(A5),
  PHYS_REG_GENERAL(A6),  PHYS_REG_GENERAL(A7),

  PHYS_REG_FLOAT(Fa0),   PHYS_REG_FLOAT(Fa1),   PHYS_REG_FLOAT(Fa2),
  PHYS_REG_FLOAT(Fa3),   PHYS_REG_FLOAT(Fa4),   PHYS_REG_FLOAT(Fa5),
  PHYS_REG_FLOAT(Fa6),   PHYS_REG_FLOAT(Fa7),

  PHYS_REG_GENERAL(T3),  PHYS_REG_GENERAL(T4),  PHYS_REG_GENERAL(T5),
  PHYS_REG_GENERAL(T6),

  PHYS_REG_FLOAT(Ft3),   PHYS_REG_FLOAT(Ft4),   PHYS_REG_FLOAT(Ft5),
  PHYS_REG_FLOAT(Ft6),   PHYS_REG_FLOAT(Ft7),   PHYS_REG_FLOAT(Ft8),
  PHYS_REG_FLOAT(Ft9),   PHYS_REG_FLOAT(Ft10),  PHYS_REG_FLOAT(Ft11),

  PHYS_REG_GENERAL(S1),  PHYS_REG_GENERAL(S2),  PHYS_REG_GENERAL(S3),
  PHYS_REG_GENERAL(S4),  PHYS_REG_GENERAL(S5),  PHYS_REG_GENERAL(S6),
  PHYS_REG_GENERAL(S7),  PHYS_REG_GENERAL(S8),  PHYS_REG_GENERAL(S9),
  PHYS_REG_GENERAL(S10), PHYS_REG_GENERAL(S11),

  PHYS_REG_FLOAT(Fs1),   PHYS_REG_FLOAT(Fs1),   PHYS_REG_FLOAT(Fs2),
  PHYS_REG_FLOAT(Fs3),   PHYS_REG_FLOAT(Fs4),   PHYS_REG_FLOAT(Fs5),
  PHYS_REG_FLOAT(Fs6),   PHYS_REG_FLOAT(Fs7),   PHYS_REG_FLOAT(Fs8),
  PHYS_REG_FLOAT(Fs9),   PHYS_REG_FLOAT(Fs10),  PHYS_REG_FLOAT(Fs11),
};

const std::vector<Register> REG_SPILL_GENERAL = {
  PHYS_REG_GENERAL(T0),
  PHYS_REG_GENERAL(T1),
};

const std::vector<Register> REG_SPILL_FLOAT = {
  PHYS_REG_FLOAT(Ft0),
  PHYS_REG_FLOAT(Ft1),
  PHYS_REG_FLOAT(Ft2),
};

const std::set<Register> REG_CALLEE_SAVED = {
  PHYS_REG_GENERAL(Zero), PHYS_REG_GENERAL(Ra),  PHYS_REG_GENERAL(Sp),
  PHYS_REG_GENERAL(Gp),   PHYS_REG_GENERAL(Tp),  PHYS_REG_GENERAL(S0),

  PHYS_REG_GENERAL(S1),   PHYS_REG_GENERAL(S2),  PHYS_REG_GENERAL(S3),
  PHYS_REG_GENERAL(S4),   PHYS_REG_GENERAL(S5),  PHYS_REG_GENERAL(S6),
  PHYS_REG_GENERAL(S7),   PHYS_REG_GENERAL(S8),  PHYS_REG_GENERAL(S9),
  PHYS_REG_GENERAL(S10),  PHYS_REG_GENERAL(S11),

  PHYS_REG_FLOAT(Fs1),    PHYS_REG_FLOAT(Fs2),   PHYS_REG_FLOAT(Fs3),
  PHYS_REG_FLOAT(Fs4),    PHYS_REG_FLOAT(Fs5),   PHYS_REG_FLOAT(Fs6),
  PHYS_REG_FLOAT(Fs7),    PHYS_REG_FLOAT(Fs8),   PHYS_REG_FLOAT(Fs9),
  PHYS_REG_FLOAT(Fs10),   PHYS_REG_FLOAT(Fs11),
};

const std::set<Register> REG_ARGS = {
  PHYS_REG_GENERAL(A0), PHYS_REG_GENERAL(A1), PHYS_REG_GENERAL(A2),
  PHYS_REG_GENERAL(A3), PHYS_REG_GENERAL(A4), PHYS_REG_GENERAL(A5),
  PHYS_REG_GENERAL(A6), PHYS_REG_GENERAL(A7),

  PHYS_REG_FLOAT(Fa0),  PHYS_REG_FLOAT(Fa1),  PHYS_REG_FLOAT(Fa2),
  PHYS_REG_FLOAT(Fa3),  PHYS_REG_FLOAT(Fa4),  PHYS_REG_FLOAT(Fa5),
  PHYS_REG_FLOAT(Fa6),  PHYS_REG_FLOAT(Fa7),
};

const std::set<Register> REG_TEMP = {
  PHYS_REG_GENERAL(T0), PHYS_REG_GENERAL(T1), PHYS_REG_GENERAL(T2),
  PHYS_REG_GENERAL(T3), PHYS_REG_GENERAL(T4), PHYS_REG_GENERAL(T5),
  PHYS_REG_GENERAL(T6),

  PHYS_REG_FLOAT(Ft0),  PHYS_REG_FLOAT(Ft1),  PHYS_REG_FLOAT(Ft2),
  PHYS_REG_FLOAT(Ft3),  PHYS_REG_FLOAT(Ft4),  PHYS_REG_FLOAT(Ft5),
  PHYS_REG_FLOAT(Ft6),  PHYS_REG_FLOAT(Ft7),  PHYS_REG_FLOAT(Ft8),
  PHYS_REG_FLOAT(Ft9),  PHYS_REG_FLOAT(Ft10), PHYS_REG_FLOAT(Ft11),
};

}  // namespace backend
}  // namespace syc

#endif