#include "backend/instruction.h"
#include "backend/basic_block.h"
#include "backend/operand.h"

namespace syc {
namespace backend {

std::string Instruction::to_string(Context& context) {
  using namespace instruction;

  return std::visit(
    overloaded{
      [&context](const Load& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto rs = context.get_operand(instruction.rs_id);
        auto imm = context.get_operand(instruction.imm_id);

        switch (instruction.op) {
          case Load::LB:
            ss << "lb";
            break;
          case Load::LH:
            ss << "lh";
            break;
          case Load::LW:
            ss << "lw";
            break;
          case Load::LBU:
            ss << "lbu";
            break;
          case Load::LHU:
            ss << "lhu";
            break;
          case Load::LD:
            ss << "ld";
            break;
          case Load::LWU:
            ss << "lwu";
            break;
        }

        ss << " " << rd->to_string() << ", " << imm->to_string() << "("
           << rs->to_string() << ")";

        return ss.str();
      },

      [&context](const Store& instruction) {
        std::stringstream ss;

        auto rs1 = context.get_operand(instruction.rs1_id);
        auto rs2 = context.get_operand(instruction.rs2_id);
        auto imm = context.get_operand(instruction.imm_id);

        switch (instruction.op) {
          case Store::SB:
            ss << "sb";
            break;
          case Store::SH:
            ss << "sh";
            break;
          case Store::SW:
            ss << "sw";
            break;
          case Store::SD:
            ss << "sd";
            break;
        }

        ss << " " << rs2->to_string() << ", " << imm->to_string() << "("
           << rs1->to_string() << ")";

        return ss.str();
      },
      [&context](const Binary& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto rs1 = context.get_operand(instruction.rs1_id);
        auto rs2 = context.get_operand(instruction.rs2_id);

        switch (instruction.op) {
          case Binary::ADD:
            ss << "add";
            break;
          case Binary::ADDW:
            ss << "addw";
            break;
          case Binary::SUB:
            ss << "sub";
            break;
          case Binary::SUBW:
            ss << "subw";
            break;
          case Binary::SLL:
            ss << "sll";
            break;
          case Binary::SLLW:
            ss << "sllw";
            break;
          case Binary::SRL:
            ss << "srl";
            break;
          case Binary::SRLW:
            ss << "srlw";
            break;
          case Binary::SRA:
            ss << "sra";
            break;
          case Binary::SRAW:
            ss << "sraw";
            break;
          case Binary::XOR:
            ss << "xor";
            break;
          case Binary::OR:
            ss << "or";
            break;
          case Binary::AND:
            ss << "and";
            break;
          case Binary::SLT:
            ss << "slt";
            break;
          case Binary::SLTU:
            ss << "sltu";
            break;
          case Binary::MUL:
            ss << "mul";
            break;
          case Binary::MULW:
            ss << "mulw";
            break;
          case Binary::MULH:
            ss << "mulh";
            break;
          case Binary::MULHSU:
            ss << "mulhsu";
            break;
          case Binary::MULHU:
            ss << "mulhu";
            break;
          case Binary::DIV:
            ss << "div";
            break;
          case Binary::DIVW:
            ss << "divw";
            break;
          case Binary::DIVU:
            ss << "divu";
            break;
          case Binary::REM:
            ss << "rem";
            break;
          case Binary::REMW:
            ss << "remw";
            break;
          case Binary::REMU:
            ss << "remu";
            break;
          case Binary::REMUW:
            ss << "remuw";
            break;
        }

        ss << " " << rd->to_string() << ", " << rs1->to_string() << ", "
           << rs2->to_string();

        return ss.str();
      },
      [&context](const BinaryImm& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto rs = context.get_operand(instruction.rs_id);
        auto imm = context.get_operand(instruction.imm_id);

        switch (instruction.op) {
          case BinaryImm::ADDI:
            ss << "addi";
            break;
          case BinaryImm::ADDIW:
            ss << "addiw";
            break;
          case BinaryImm::SLLI:
            ss << "slli";
            break;
          case BinaryImm::SLLIW:
            ss << "slliw";
            break;
          case BinaryImm::SRLI:
            ss << "srli";
            break;
          case BinaryImm::SRLIW:
            ss << "srliw";
            break;
          case BinaryImm::SRAI:
            ss << "srai";
            break;
          case BinaryImm::SRAIW:
            ss << "sraiw";
            break;
          case BinaryImm::XORI:
            ss << "xori";
            break;
          case BinaryImm::ORI:
            ss << "ori";
            break;
          case BinaryImm::ANDI:
            ss << "andi";
            break;
          case BinaryImm::SLTI:
            ss << "slti";
            break;
          case BinaryImm::SLTIU:
            ss << "sltiu";
            break;
        }

        ss << " " << rd->to_string() << ", " << rs->to_string() << ", "
           << imm->to_string();

        return ss.str();
      },
      [&context](const Lui& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto imm = context.get_operand(instruction.imm_id);

        ss << "lui " << rd->to_string() << ", " << imm->to_string();

        return ss.str();
      },
      [&context](const Li& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto imm = context.get_operand(instruction.imm_id);

        ss << "li " << rd->to_string() << ", " << imm->to_string();

        return ss.str();
      },
      [&context](const Call& instruction) {
        return "call " + instruction.function_name;
      },
      [&context](const Branch& instruction) {
        std::stringstream ss;

        auto rs1 = context.get_operand(instruction.rs1_id);
        auto rs2 = context.get_operand(instruction.rs2_id);

        switch (instruction.op) {
          case Branch::BEQ:
            ss << "beq";
            break;
          case Branch::BNE:
            ss << "bne";
            break;
          case Branch::BLT:
            ss << "blt";
            break;
          case Branch::BGE:
            ss << "bge";
            break;
          case Branch::BLTU:
            ss << "bltu";
            break;
          case Branch::BGEU:
            ss << "bgeu";
            break;
        }

        ss << " " << rs1->to_string() << ", " << rs2->to_string() << ", "
           << context.get_basic_block(instruction.block_id)->get_label();

        return ss.str();
      },
      [&context](const FloatLoad& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto rs = context.get_operand(instruction.rs_id);
        auto imm = context.get_operand(instruction.imm_id);

        switch (instruction.op) {
          case FloatLoad::FLW:
            ss << "flw";
            break;
          case FloatLoad::FLD:
            ss << "fld";
            break;
        }

        ss << " " << rd->to_string() << ", " << imm << "(" << rs->to_string()
           << ")";

        return ss.str();
      },
      [&context](const FloatStore& instruction) {
        std::stringstream ss;

        auto rs1 = context.get_operand(instruction.rs1_id);
        auto rs2 = context.get_operand(instruction.rs2_id);
        auto imm = context.get_operand(instruction.imm_id);

        switch (instruction.op) {
          case FloatStore::FSW:
            ss << "fsw";
            break;
          case FloatStore::FSD:
            ss << "fsd";
            break;
        }

        ss << " " << rs2->to_string() << ", " << imm << "(" << rs1->to_string()
           << ")";

        return ss.str();
      },
      [&context](const FloatMove& instruction) {
        std::stringstream ss;

        ss << "fmv";

        switch (instruction.dst_fmt) {
          case FloatMove::H:
            ss << ".h";
            break;
          case FloatMove::S:
            ss << ".s";
            break;
          case FloatMove::D:
            ss << ".d";
            break;
          case FloatMove::X:
            ss << ".x";
            break;
        }

        switch (instruction.src_fmt) {
          case FloatMove::H:
            ss << ".h";
            break;
          case FloatMove::S:
            ss << ".s";
            break;
          case FloatMove::D:
            ss << ".d";
            break;
          case FloatMove::X:
            ss << ".x";
            break;
        }

        auto rd = context.get_operand(instruction.rd_id);
        auto rs = context.get_operand(instruction.rs_id);

        ss << " " << rd->to_string() << ", " << rs->to_string();

        return ss.str();
      },
      [&context](const FloatConvert& instruction) {
        std::stringstream ss;

        ss << "fcvt";

        switch (instruction.dst_fmt) {
          case FloatConvert::H:
            ss << ".h";
            break;
          case FloatConvert::S:
            ss << ".s";
            break;
          case FloatConvert::D:
            ss << ".d";
            break;
          case FloatConvert::W:
            ss << ".w";
            break;
          case FloatConvert::WU:
            ss << ".wu";
            break;
          case FloatConvert::L:
            ss << ".l";
            break;
          case FloatConvert::LU:
            ss << ".lu";
            break;
        }

        switch (instruction.src_fmt) {
          case FloatConvert::H:
            ss << ".h";
            break;
          case FloatConvert::S:
            ss << ".s";
            break;
          case FloatConvert::D:
            ss << ".d";
            break;
          case FloatConvert::W:
            ss << ".w";
            break;
          case FloatConvert::WU:
            ss << ".wu";
            break;
          case FloatConvert::L:
            ss << ".l";
            break;
          case FloatConvert::LU:
            ss << ".lu";
            break;
        }

        auto rd = context.get_operand(instruction.rd_id);
        auto rs = context.get_operand(instruction.rs_id);

        ss << " " << rd->to_string() << ", " << rs->to_string();

        return ss.str();
      },
      [&context](const FloatBinary& instruction) {
        std::stringstream ss;

        switch (instruction.op) {
          case FloatBinary::FADD:
            ss << "fadd";
            break;
          case FloatBinary::FSUB:
            ss << "fsub";
            break;
          case FloatBinary::FMUL:
            ss << "fmul";
            break;
          case FloatBinary::FDIV:
            ss << "fdiv";
            break;
          case FloatBinary::FSGNJ:
            ss << "fsgnj";
            break;
          case FloatBinary::FSGNJN:
            ss << "fsgnjn";
            break;
          case FloatBinary::FSGNJX:
            ss << "fsgnjx";
            break;
          case FloatBinary::FMIN:
            ss << "fmin";
            break;
          case FloatBinary::FMAX:
            ss << "fmax";
            break;
          case FloatBinary::FEQ:
            ss << "feq";
            break;
          case FloatBinary::FLT:
            ss << "flt";
            break;
          case FloatBinary::FLE:
            ss << "fle";
            break;
        }

        switch (instruction.fmt) {
          case FloatBinary::S:
            ss << ".s";
            break;
          case FloatBinary::D:
            ss << ".d";
            break;
        }

        auto rd = context.get_operand(instruction.rd_id);
        auto rs1 = context.get_operand(instruction.rs1_id);
        auto rs2 = context.get_operand(instruction.rs2_id);

        ss << " " << rd->to_string() << ", " << rs1->to_string() << ", "
           << rs2->to_string();

        return ss.str();
      },
      [&context](const FloatMulAdd& instruction) {
        std::stringstream ss;

        switch (instruction.op) {
          case FloatMulAdd::FMADD:
            ss << "fmadd";
            break;
          case FloatMulAdd::FMSUB:
            ss << "fmsub";
            break;
          case FloatMulAdd::FNMSUB:
            ss << "fnmsub";
            break;
          case FloatMulAdd::FNMADD:
            ss << "fnmadd";
            break;
        }

        switch (instruction.fmt) {
          case FloatMulAdd::S:
            ss << ".s";
            break;
          case FloatMulAdd::D:
            ss << ".d";
            break;
        }

        auto rd = context.get_operand(instruction.rd_id);
        auto rs1 = context.get_operand(instruction.rs1_id);
        auto rs2 = context.get_operand(instruction.rs2_id);
        auto rs3 = context.get_operand(instruction.rs3_id);

        ss << " " << rd->to_string() << ", " << rs1->to_string() << ", "
           << rs2->to_string() << ", " << rs3->to_string();

        return ss.str();
      },
      [&context](const FloatUnary& instruction) {
        std::stringstream ss;

        switch (instruction.op) {
          case FloatUnary::FSQRT:
            ss << "fsqrt";
            break;
          case FloatUnary::FCLASS:
            ss << "fclass";
            break;
        }

        switch (instruction.fmt) {
          case FloatUnary::S:
            ss << ".s";
            break;
          case FloatUnary::D:
            ss << ".d";
            break;
        }

        auto rd = context.get_operand(instruction.rd_id);
        auto rs = context.get_operand(instruction.rs_id);

        ss << " " << rd->to_string() << ", " << rs->to_string();

        return ss.str();
      },
    },
    kind
  );
}

}  // namespace backend
}  // namespace syc