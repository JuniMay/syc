#ifndef SYC_BACKEND_INSTRUCTION_H_
#define SYC_BACKEND_INSTRUCTION_H_

#include "backend/context.h"
#include "common.h"

namespace syc {
namespace backend {

namespace instruction {

struct Load {
  enum Op {
    /// Load byte
    LB,
    /// Load half word
    LH,
    /// Load word
    LW,
    /// Load byte unsigned
    LBU,
    /// Load half word unsigned
    LHU,
    /// Load double word
    LD,
    /// Load word unsigned
    LWU,
  };
  Op op;
  /// Destination register
  OperandID rd_id;
  /// Source register
  OperandID rs_id;
  /// Immediate
  OperandID imm_id;
};

struct FloatLoad {
  enum Op {
    FLW,
    FLD,
  };

  Op op;
  OperandID rd_id;
  OperandID rs_id;
  OperandID imm_id;
};

/// Store instruction
/// Note that in S-type instruction, rs1 is the base register and rs2 is the
/// source register.
struct Store {
  enum Op {
    /// Store byte
    SB,
    /// Store half word
    SH,
    /// Store word
    SW,
    /// Store double word
    SD,
  };
  Op op;
  /// Base register
  OperandID rs1_id;
  /// Source resgiter
  OperandID rs2_id;
  /// Immediate
  OperandID imm_id;
};

struct FloatStore {
  enum Op {
    FSW,
    FSD,
  };

  Op op;

  OperandID rs1_id;
  OperandID rs2_id;
  OperandID imm_id;
};

struct FloatMove {
  enum Fmt {
    H,
    S,
    D,
    X,
  };

  Fmt dst_fmt;
  Fmt src_fmt;

  OperandID rd_id;
  OperandID rs_id;
};

struct FloatConvert {
  enum Fmt {
    H,
    S,
    D,
    W,
    WU,
    L,
    LU,
  };

  Fmt dst_fmt;
  Fmt src_fmt;

  OperandID rd_id;
  OperandID rs_id;
};

struct Binary {
  enum Op {
    ADD,
    ADDW,
    SUB,
    SUBW,

    SLL,
    SLLW,
    SRL,
    SRLW,
    SRA,
    SRAW,

    XOR,
    OR,
    AND,

    SLT,
    SLTU,

    MUL,
    MULW,
    MULH,
    MULHSU,
    MULHU,
    DIV,
    DIVW,
    DIVU,
    REM,
    REMW,
    REMU,
    REMUW,
  };
  Op op;
  /// Destination register
  OperandID rd_id;
  /// Source register
  OperandID rs1_id;
  /// Source register
  OperandID rs2_id;
};

struct BinaryImm {
  enum Op {
    ADDI,
    ADDIW,

    SLLI,
    SLLIW,
    SRLI,
    SRLIW,
    SRAI,
    SRAIW,

    XORI,
    ORI,
    ANDI,
    SLTI,
    SLTIU,
  };
  Op op;
  /// Destination register
  OperandID rd_id;
  /// Source register
  OperandID rs_id;
  /// Immediate
  OperandID imm_id;
};

struct FloatBinary {
  enum Op {
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FSGNJ,
    FSGNJN,
    FSGNJX,
    FMIN,
    FMAX,
    FEQ,
    FLT,
    FLE,
  };

  enum Fmt {
    S,
    D,
  };

  Op op;
  Fmt fmt;
  /// Destination register
  OperandID rd_id;
  /// Source register
  OperandID rs1_id;
  /// Source register
  OperandID rs2_id;
};

struct FloatMulAdd {
  enum Op {
    FMADD,
    FMSUB,
    FNMSUB,
    FNMADD,
  };

  enum Fmt {
    S,
    D,
  };

  Op op;
  Fmt fmt;

  OperandID rd_id;
  OperandID rs1_id;
  OperandID rs2_id;
  OperandID rs3_id;
};

struct FloatUnary {
  enum Op {
    FSQRT,
    FCLASS,
  };

  enum Fmt {
    S,
    D,
  };

  Op op;
  Fmt fmt;
  /// Destination register
  OperandID rd_id;
  /// Source register
  OperandID rs_id;
};

/// Load upper immediate
struct Lui {
  /// Destination register
  OperandID rd_id;
  /// Immediate
  OperandID imm_id;
};

/// Load immediate (pseudo instruction)
struct Li {
  /// Destination register
  OperandID rd_id;
  /// Immediate
  OperandID imm_id;
};

/// Call function (pseudo instruction)
struct Call {
  /// Function name
  std::string function_name;
};

struct Branch {
  enum Op {
    BEQ,
    BNE,
    BLT,
    BGE,
    BLTU,
    BGEU,
  };
  Op op;
  OperandID rs1_id;
  OperandID rs2_id;
  /// Block label;
  BasicBlockID block_id;
};

}  // namespace instruction

struct Instruction {
  InstructionID id;
  InstructionKind kind;

  BasicBlockID parent_block_id;

  std::string to_string(Context& context);
};

}  // namespace backend
}  // namespace syc

#endif