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

struct PseudoLoad {
  enum Op {
    LA,
    LW,
  };

  Op op;
  OperandID rd_id;
  /// This is usually global.
  OperandID symbol_id;
};

struct PseudoStore {
  enum Op {
    SW,
  };

  Op op;
  OperandID rd_id;
  OperandID symbol_id;
  OperandID rt_id;
};

struct FloatPseudoLoad {
  enum Op {
    FLW,
  };

  Op op;
  OperandID rd_id;
  OperandID symbol_id;
  OperandID rt_id;
};

struct FloatPseudoStore {
  enum Op {
    FSW,
  };

  Op op;
  OperandID rd_id;
  OperandID symbol_id;
  OperandID rt_id;
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

struct Ret {};

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

struct J {
  BasicBlockID block_id;
};

struct Dummy {};

}  // namespace instruction

/// Machine instruction
struct Instruction : std::enable_shared_from_this<Instruction> {
  /// Instruction ID in the context.
  InstructionID id;
  /// Instruction kind.
  InstructionKind kind;
  /// ID of the parent basic block.
  BasicBlockID parent_block_id;

  /// Next instruction.
  InstructionPtr next;
  /// Previous instruction.
  InstructionPrevPtr prev;

  std::vector<OperandID> def_id_list;
  std::vector<OperandID> use_id_list;

  void add_def(OperandID def_id);
  void add_use(OperandID use_id);

  /// Constructor
  Instruction(
    InstructionID id,
    InstructionKind kind,
    BasicBlockID parent_block_id
  );

  /// Insert instruction to the next of the current instruction.
  void insert_next(InstructionPtr instruction);
  /// Insert instruction to the prev of the current instruction.
  void insert_prev(InstructionPtr instruction);

  void remove(Context& context);

  /// Convert the instruction to a string of assembly code.
  std::string to_string(Context& context);

  std::optional<BasicBlockID> get_basic_block_id_if_branch() const;

  void replace_operand(OperandID old_operand_id, OperandID new_operand_id, Context& context);
};

InstructionPtr create_instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
);

InstructionPtr create_dummy_instruction();

}  // namespace backend
}  // namespace syc

#endif