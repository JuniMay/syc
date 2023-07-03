#include "backend/register.h"

namespace syc {
namespace backend {

std::string Register::to_string() const {
  std::string result = "";

  if (is_general()) {
    switch (std::get<GeneralRegister>(reg)) {
      case GeneralRegister::Zero:
        result = "zero";
        break;
      case GeneralRegister::Ra:
        result = "ra";
        break;
      case GeneralRegister::Sp:
        result = "sp";
        break;
      case GeneralRegister::Gp:
        result = "gp";
        break;
      case GeneralRegister::Tp:
        result = "tp";
        break;
      case GeneralRegister::T0:
        result = "t0";
        break;
      case GeneralRegister::T1:
        result = "t1";
        break;
      case GeneralRegister::T2:
        result = "t2";
        break;
      case GeneralRegister::S0:
        result = "s0";
        break;
      case GeneralRegister::S1:
        result = "s1";
        break;
      case GeneralRegister::A0:
        result = "a0";
        break;
      case GeneralRegister::A1:
        result = "a1";
        break;
      case GeneralRegister::A2:
        result = "a2";
        break;
      case GeneralRegister::A3:
        result = "a3";
        break;
      case GeneralRegister::A4:
        result = "a4";
        break;
      case GeneralRegister::A5:
        result = "a5";
        break;
      case GeneralRegister::A6:
        result = "a6";
        break;
      case GeneralRegister::A7:
        result = "a7";
        break;
      case GeneralRegister::S2:
        result = "s2";
        break;
      case GeneralRegister::S3:
        result = "s3";
        break;
      case GeneralRegister::S4:
        result = "s4";
        break;
      case GeneralRegister::S5:
        result = "s5";
        break;
      case GeneralRegister::S6:
        result = "s6";
        break;
      case GeneralRegister::S7:
        result = "s7";
        break;
      case GeneralRegister::S8:
        result = "s8";
        break;
      case GeneralRegister::S9:
        result = "s9";
        break;
      case GeneralRegister::S10:
        result = "s10";
        break;
      case GeneralRegister::S11:
        result = "s11";
        break;
      case GeneralRegister::T3:
        result = "t3";
        break;
      case GeneralRegister::T4:
        result = "t4";
        break;
      case GeneralRegister::T5:
        result = "t5";
        break;
      case GeneralRegister::T6:
        result = "t6";
        break;
    }
  } else if (is_float()) {
    switch (std::get<FloatRegister>(reg)) {
      case FloatRegister::Ft0:
        result = "ft0";
        break;
      case FloatRegister::Ft1:
        result = "ft1";
        break;
      case FloatRegister::Ft2:
        result = "ft2";
        break;
      case FloatRegister::Ft3:
        result = "ft3";
        break;
      case FloatRegister::Ft4:
        result = "ft4";
        break;
      case FloatRegister::Ft5:
        result = "ft5";
        break;
      case FloatRegister::Ft6:
        result = "ft6";
        break;
      case FloatRegister::Ft7:
        result = "ft7";
        break;
      case FloatRegister::Fs0:
        result = "fs0";
        break;
      case FloatRegister::Fs1:
        result = "fs1";
        break;
      case FloatRegister::Fa0:
        result = "fa0";
        break;
      case FloatRegister::Fa1:
        result = "fa1";
        break;
      case FloatRegister::Fa2:
        result = "fa2";
        break;
      case FloatRegister::Fa3:
        result = "fa3";
        break;
      case FloatRegister::Fa4:
        result = "fa4";
        break;
      case FloatRegister::Fa5:
        result = "fa5";
        break;
      case FloatRegister::Fa6:
        result = "fa6";
        break;
      case FloatRegister::Fa7:
        result = "fa7";
        break;
      case FloatRegister::Fs2:
        result = "fs2";
        break;
      case FloatRegister::Fs3:
        result = "fs3";
        break;
      case FloatRegister::Fs4:
        result = "fs4";
        break;
      case FloatRegister::Fs5:
        result = "fs5";
        break;
      case FloatRegister::Fs6:
        result = "fs6";
        break;
      case FloatRegister::Fs7:
        result = "fs7";
        break;
      case FloatRegister::Fs8:
        result = "fs8";
        break;
      case FloatRegister::Fs9:
        result = "fs9";
        break;
      case FloatRegister::Fs10:
        result = "fs10";
        break;
      case FloatRegister::Fs11:
        result = "fs11";
        break;
      case FloatRegister::Ft8:
        result = "ft8";
        break;
      case FloatRegister::Ft9:
        result = "ft9";
        break;
      case FloatRegister::Ft10:
        result = "ft10";
        break;
      case FloatRegister::Ft11:
        result = "ft11";
        break;
    }
  }

  return result;
}

std::string VirtualRegister::to_string() const {
  std::string result = "";
  if (is_general()) {
    result = "v" + std::to_string(id);
  } else if (is_float()) {
    result = "fv" + std::to_string(id);
  }
  return result;
}

}  // namespace backend
}  // namespace syc