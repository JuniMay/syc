#include "backend__register.h"

namespace syc {
namespace backend {

std::string Register::to_string() const {
  std::string result = "";

  if (is_gp()) {
    switch (std::get<GPRegister>(reg)) {
      case GPRegister::Zero:
        result = "zero";
        break;
      case GPRegister::Ra:
        result = "ra";
        break;
      case GPRegister::Sp:
        result = "sp";
        break;
      case GPRegister::Gp:
        result = "gp";
        break;
      case GPRegister::Tp:
        result = "tp";
        break;
      case GPRegister::T0:
        result = "t0";
        break;
      case GPRegister::T1:
        result = "t1";
        break;
      case GPRegister::T2:
        result = "t2";
        break;
      case GPRegister::S0:
        result = "s0";
        break;
      case GPRegister::S1:
        result = "s1";
        break;
      case GPRegister::A0:
        result = "a0";
        break;
      case GPRegister::A1:
        result = "a1";
        break;
      case GPRegister::A2:
        result = "a2";
        break;
      case GPRegister::A3:
        result = "a3";
        break;
      case GPRegister::A4:
        result = "a4";
        break;
      case GPRegister::A5:
        result = "a5";
        break;
      case GPRegister::A6:
        result = "a6";
        break;
      case GPRegister::A7:
        result = "a7";
        break;
      case GPRegister::S2:
        result = "s2";
        break;
      case GPRegister::S3:
        result = "s3";
        break;
      case GPRegister::S4:
        result = "s4";
        break;
      case GPRegister::S5:
        result = "s5";
        break;
      case GPRegister::S6:
        result = "s6";
        break;
      case GPRegister::S7:
        result = "s7";
        break;
      case GPRegister::S8:
        result = "s8";
        break;
      case GPRegister::S9:
        result = "s9";
        break;
      case GPRegister::S10:
        result = "s10";
        break;
      case GPRegister::S11:
        result = "s11";
        break;
      case GPRegister::T3:
        result = "t3";
        break;
      case GPRegister::T4:
        result = "t4";
        break;
      case GPRegister::T5:
        result = "t5";
        break;
      case GPRegister::T6:
        result = "t6";
        break;
    }
  } else if (is_fp()) {
    switch (std::get<FPRegister>(reg)) {
      case FPRegister::Ft0:
        result = "ft0";
        break;
      case FPRegister::Ft1:
        result = "ft1";
        break;
      case FPRegister::Ft2:
        result = "ft2";
        break;
      case FPRegister::Ft3:
        result = "ft3";
        break;
      case FPRegister::Ft4:
        result = "ft4";
        break;
      case FPRegister::Ft5:
        result = "ft5";
        break;
      case FPRegister::Ft6:
        result = "ft6";
        break;
      case FPRegister::Ft7:
        result = "ft7";
        break;
      case FPRegister::Fs0:
        result = "fs0";
        break;
      case FPRegister::Fs1:
        result = "fs1";
        break;
      case FPRegister::Fa0:
        result = "fa0";
        break;
      case FPRegister::Fa1:
        result = "fa1";
        break;
      case FPRegister::Fa2:
        result = "fa2";
        break;
      case FPRegister::Fa3:
        result = "fa3";
        break;
      case FPRegister::Fa4:
        result = "fa4";
        break;
      case FPRegister::Fa5:
        result = "fa5";
        break;
      case FPRegister::Fa6:
        result = "fa6";
        break;
      case FPRegister::Fa7:
        result = "fa7";
        break;
      case FPRegister::Fs2:
        result = "fs2";
        break;
      case FPRegister::Fs3:
        result = "fs3";
        break;
      case FPRegister::Fs4:
        result = "fs4";
        break;
      case FPRegister::Fs5:
        result = "fs5";
        break;
      case FPRegister::Fs6:
        result = "fs6";
        break;
      case FPRegister::Fs7:
        result = "fs7";
        break;
      case FPRegister::Fs8:
        result = "fs8";
        break;
      case FPRegister::Fs9:
        result = "fs9";
        break;
      case FPRegister::Fs10:
        result = "fs10";
        break;
      case FPRegister::Fs11:
        result = "fs11";
        break;
      case FPRegister::Ft8:
        result = "ft8";
        break;
      case FPRegister::Ft9:
        result = "ft9";
        break;
      case FPRegister::Ft10:
        result = "ft10";
        break;
      case FPRegister::Ft11:
        result = "ft11";
        break;
    }
  }

  return result;
}

std::string VirtualRegister::to_string() const {
  std::string result = "";
  if (is_gp()) {
    result = "v" + std::to_string(id);
  } else if (is_fp()) {
    result = "fv" + std::to_string(id);
  }
  return result;
}

}  // namespace backend
}  // namespace syc