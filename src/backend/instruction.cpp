#include "backend/instruction.h"
#include "backend/basic_block.h"
#include "backend/operand.h"

namespace syc {
namespace backend {

Instruction::Instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
)
  : id(id), kind(kind), parent_block_id(parent_block_id), next(nullptr) {}

void Instruction::add_def(OperandID def_id) {
  def_id_list.push_back(def_id);
}

void Instruction::add_use(OperandID use_id) {
  use_id_list.push_back(use_id);
}

void Instruction::insert_next(InstructionPtr instruction) {
  instruction->next = this->next;
  instruction->prev = this->shared_from_this();

  if (this->next) {
    this->next->prev = instruction;
  }
  this->next = instruction;
}

void Instruction::insert_prev(InstructionPtr instruction) {
  instruction->next = this->shared_from_this();
  instruction->prev = this->prev;

  if (auto prev = this->prev.lock()) {
    prev->next = instruction;
  }

  this->prev = instruction;
}

std::optional<BasicBlockID> Instruction::get_basic_block_id_if_branch() const {
  if (auto branch = std::get_if<instruction::Branch>(&this->kind)) {
    return branch->block_id;
  } else if (auto jmp = std::get_if<instruction::J>(&this->kind)) {
    return jmp->block_id;
  } else {
    return std::nullopt;
  }
}

InstructionPtr create_instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
) {
  return std::make_shared<Instruction>(id, kind, parent_block_id);
}

InstructionPtr create_dummy_instruction() {
  return create_instruction(
    std::numeric_limits<InstructionID>::max(), instruction::Dummy{},
    std::numeric_limits<BasicBlockID>::max()
  );
}

void Instruction::replace_operand(
  OperandID old_operand_id,
  OperandID new_operand_id,
  Context& context
) {
  using namespace instruction;

  if (old_operand_id == new_operand_id) {
    return;
  }

  auto old_operand = context.get_operand(old_operand_id);
  auto new_operand = context.get_operand(new_operand_id);

  if (std::find(this->def_id_list.begin(), this->def_id_list.end(), old_operand_id) != this->def_id_list.end()) {
    this->def_id_list.erase(std::find(
      this->def_id_list.begin(), this->def_id_list.end(), old_operand_id
    ));
    this->def_id_list.push_back(new_operand_id);
    old_operand->remove_def(this->id);
    new_operand->add_def(this->id);
  }

  if (std::find(this->use_id_list.begin(), this->use_id_list.end(), old_operand_id) != this->use_id_list.end()) {
    this->use_id_list.erase(std::find(
      this->use_id_list.begin(), this->use_id_list.end(), old_operand_id
    ));
    this->use_id_list.push_back(new_operand_id);
    old_operand->remove_use(this->id);
    new_operand->add_use(this->id);
  }

  std::visit(
    overloaded{
      [&](Load& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatLoad& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](PseudoLoad& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
      },
      [&](FloatPseudoLoad& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](PseudoStore& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](FloatPseudoStore& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](Store& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatStore& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatMove& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
      },
      [&](FloatConvert& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
      },
      [&](Binary& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
      },
      [&](BinaryImm& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatBinary& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
      },
      [&](FloatMulAdd& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
        if (instruction.rs3_id == old_operand_id) {
          instruction.rs3_id = new_operand_id;
        }
      },
      [&](FloatUnary& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
      },
      [&](Lui& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](Li& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](Branch& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
      },
      [&](Phi& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        for (auto& [id, _] : instruction.incoming_list) {
          if (id == old_operand_id) {
            id = new_operand_id;
          }
        }
      },
      [&](auto& instruction) {
        // Do nothing.
      }},
    kind
  );
}

void Instruction::replace_def_operand(
  OperandID old_operand_id,
  OperandID new_operand_id,
  Context& context
) {
  using namespace instruction;

  if (old_operand_id == new_operand_id) {
    return;
  }

  auto old_operand = context.get_operand(old_operand_id);
  auto new_operand = context.get_operand(new_operand_id);

  if (std::find(this->def_id_list.begin(), this->def_id_list.end(), old_operand_id) != this->def_id_list.end()) {
    this->def_id_list.erase(std::find(
      this->def_id_list.begin(), this->def_id_list.end(), old_operand_id
    ));
    this->def_id_list.push_back(new_operand_id);
    old_operand->remove_def(this->id);
    new_operand->add_def(this->id);
  } else {
    return;
  }

  std::visit(
    overloaded{
      [&](Load& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](FloatLoad& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](PseudoLoad& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](FloatPseudoLoad& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](PseudoStore& instruction) {
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](FloatPseudoStore& instruction) {
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](Store& instruction) {
        // Do nothing.
      },
      [&](FloatStore& instruction) {
        // Do nothing.
      },
      [&](FloatMove& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](FloatConvert& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](Binary& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](BinaryImm& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](FloatBinary& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](FloatMulAdd& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](FloatUnary& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](Lui& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](Li& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](Branch& instruction) {
        // Do nothing.
      },
      [&](Phi& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
      },
      [&](auto& instruction) {
        // Do nothing.
      }},
    kind
  );
}

void Instruction::replace_use_operand(
  OperandID old_operand_id,
  OperandID new_operand_id,
  Context& context
) {
  using namespace instruction;

  if (old_operand_id == new_operand_id) {
    return;
  }

  auto old_operand = context.get_operand(old_operand_id);
  auto new_operand = context.get_operand(new_operand_id);

  if (std::find(this->use_id_list.begin(), this->use_id_list.end(), old_operand_id) != this->use_id_list.end()) {
    this->use_id_list.erase(std::find(
      this->use_id_list.begin(), this->use_id_list.end(), old_operand_id
    ));
    this->use_id_list.push_back(new_operand_id);
    old_operand->remove_use(this->id);
    new_operand->add_use(this->id);
  } else {
    return;
  }

  std::visit(
    overloaded{
      [&](Load& instruction) {
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatLoad& instruction) {
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](PseudoLoad& instruction) {
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
      },
      [&](FloatPseudoLoad& instruction) {
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](PseudoStore& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](FloatPseudoStore& instruction) {
        if (instruction.rd_id == old_operand_id) {
          instruction.rd_id = new_operand_id;
        }
        if (instruction.symbol_id == old_operand_id) {
          instruction.symbol_id = new_operand_id;
        }
        if (instruction.rt_id == old_operand_id) {
          instruction.rt_id = new_operand_id;
        }
      },
      [&](Store& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatStore& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatMove& instruction) {
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
      },
      [&](FloatConvert& instruction) {
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
      },
      [&](Binary& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
      },
      [&](BinaryImm& instruction) {
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](FloatBinary& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
      },
      [&](FloatMulAdd& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
        if (instruction.rs3_id == old_operand_id) {
          instruction.rs3_id = new_operand_id;
        }
      },
      [&](FloatUnary& instruction) {
        if (instruction.rs_id == old_operand_id) {
          instruction.rs_id = new_operand_id;
        }
      },
      [&](Lui& instruction) {
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](Li& instruction) {
        if (instruction.imm_id == old_operand_id) {
          instruction.imm_id = new_operand_id;
        }
      },
      [&](Branch& instruction) {
        if (instruction.rs1_id == old_operand_id) {
          instruction.rs1_id = new_operand_id;
        }
        if (instruction.rs2_id == old_operand_id) {
          instruction.rs2_id = new_operand_id;
        }
      },
      [&](Phi& instruction) {
        for (auto& [id, _] : instruction.incoming_list) {
          if (id == old_operand_id) {
            id = new_operand_id;
          }
        }
      },
      [&](auto& instruction) {
        // Do nothing.
      }},
    kind
  );
}

void Instruction::remove(Context& context) {
  for (auto def_id : this->def_id_list) {
    auto def = context.get_operand(def_id);
    def->remove_def(this->id);
  }

  for (auto use_id : this->use_id_list) {
    auto use = context.get_operand(use_id);
    use->remove_use(this->id);
  }

  if (auto prev = this->prev.lock()) {
    prev->next = this->next;
  }

  if (this->next) {
    this->next->prev = this->prev;
  }
}

void Instruction::raw_remove() {
  if (auto prev = this->prev.lock()) {
    prev->next = this->next;
  }
  if (this->next) {
    this->next->prev = this->prev;
  }
}

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
      [&context](const PseudoLoad& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto symbol = context.get_operand(instruction.symbol_id);

        switch (instruction.op) {
          case PseudoLoad::LA:
            ss << "la";
            break;
          case PseudoLoad::LW:
            ss << "lw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string();

        return ss.str();
      },
      [&context](const PseudoStore& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto symbol = context.get_operand(instruction.symbol_id);
        auto rt = context.get_operand(instruction.rt_id);

        switch (instruction.op) {
          case PseudoStore::SW:
            ss << "sw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string() << ", "
           << rt->to_string();

        return ss.str();
      },
      [&context](const FloatPseudoLoad& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto symbol = context.get_operand(instruction.symbol_id);
        auto rt = context.get_operand(instruction.rt_id);

        switch (instruction.op) {
          case FloatPseudoLoad::FLW:
            ss << "flw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string() << ", "
           << rt->to_string();

        return ss.str();
      },
      [&context](const FloatPseudoStore& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto symbol = context.get_operand(instruction.symbol_id);
        auto rt = context.get_operand(instruction.rt_id);

        switch (instruction.op) {
          case FloatPseudoStore::FSW:
            ss << "fsw";
            break;
        }

        ss << " " << rd->to_string() << ", " << symbol->to_string() << ", "
           << rt->to_string();

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
           << imm->to_string(3);

        return ss.str();
      },
      [&context](const Lui& instruction) {
        std::stringstream ss;

        auto rd = context.get_operand(instruction.rd_id);
        auto imm = context.get_operand(instruction.imm_id);

        ss << "lui " << rd->to_string() << ", " << imm->to_string(5);

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

        ss << " " << rd->to_string() << ", " << imm->to_string() << "("
           << rs->to_string() << ")";

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

        ss << " " << rs2->to_string() << ", " << imm->to_string() << "("
           << rs1->to_string() << ")";

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
            ss << ".w";
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
            ss << ".w";
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

        if (instruction.dst_fmt == FloatConvert::W || 
            instruction.dst_fmt == FloatConvert::WU ||
            instruction.dst_fmt == FloatConvert::L ||
            instruction.dst_fmt == FloatConvert::LU) {
          ss << ", rtz";
        } else {
          // Rounding mode is not added if convert from int to float.
        }

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
      [&context](const J& instruction) -> std::string {
        return "j " +
               context.get_basic_block(instruction.block_id)->get_label();
      },
      [&context](const Ret& instruction) -> std::string { return "ret"; },
      [&context](const Phi& instruction) {
        std::stringstream ss;

        // make as comment
        ss << "# phi " << context.get_operand(instruction.rd_id)->to_string()
           << " = ";

        for (auto& [operand_id, block_id] : instruction.incoming_list) {
          ss << "[" << context.get_operand(operand_id)->to_string() << ", "
             << context.get_basic_block(block_id)->get_label() << "], ";
        }

        return ss.str();
      },
      [&context](const auto& instruction) -> std::string { return ""; },
    },
    kind
  );
}

bool Instruction::is_phi() const {
  return std::holds_alternative<instruction::Phi>(this->kind);
}

bool Instruction::is_branch_or_jmp() const {
  return std::holds_alternative<instruction::J>(this->kind) ||
         std::holds_alternative<instruction::Branch>(this->kind);
}

bool Instruction::is_binary() const {
  return std::holds_alternative<instruction::Binary>(this->kind);
}

bool Instruction::is_binary_imm() const {
  return std::holds_alternative<instruction::BinaryImm>(this->kind);
}

bool Instruction::is_load() const {
  return std::holds_alternative<instruction::Load>(this->kind);
}

bool Instruction::is_store() const {
  return std::holds_alternative<instruction::Store>(this->kind);
}

bool Instruction::is_float_load() const {
  return std::holds_alternative<instruction::FloatLoad>(this->kind);
}

bool Instruction::is_float_store() const {
  return std::holds_alternative<instruction::FloatStore>(this->kind);
}

bool Instruction::is_li() const {
  return std::holds_alternative<instruction::Li>(this->kind);
}

bool Instruction::is_lui() const {
  return std::holds_alternative<instruction::Lui>(this->kind);
}
}  // namespace backend
}  // namespace syc