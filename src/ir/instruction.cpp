#include "ir/instruction.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/function.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {
namespace ir {

InstructionPtr create_instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
) {
  return std::make_shared<Instruction>(id, kind, parent_block_id);
}

InstructionPtr create_dummy_instruction() {
  return create_instruction(
    std::numeric_limits<OperandID>::max(), instruction::Dummy{},
    std::numeric_limits<BasicBlockID>::max()
  );
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

void Instruction::set_def(OperandID def_id) {
  this->maybe_def_id = def_id;
}

void Instruction::add_use(OperandID use_id) {
  if (std::find(this->use_id_list.begin(), this->use_id_list.end(), use_id) ==
      this->use_id_list.end()) {
    this->use_id_list.push_back(use_id);
  } else {
    return;
  }
}

void Instruction::remove_use(OperandID use_id) {
  this->use_id_list.erase(
    std::remove(this->use_id_list.begin(), this->use_id_list.end(), use_id),
    this->use_id_list.end()
  );
}

void Instruction::replace_operand(
  OperandID old_operand_id,
  OperandID new_operand_id,
  Context& context
) {
  if (this->maybe_def_id.has_value() && this->maybe_def_id.value() == old_operand_id) {
    this->maybe_def_id = new_operand_id;
    context.operand_table[old_operand_id]->remove_def();
    context.operand_table[new_operand_id]->set_def(this->id);
  }

  for (auto& use_id : this->use_id_list) {
    if (use_id == old_operand_id) {
      use_id = new_operand_id;
      context.operand_table[old_operand_id]->remove_use(this->id);
      context.operand_table[new_operand_id]->add_use(this->id);
    }
  }

  std::visit(
    overloaded{
      [&](instruction::Binary& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        if (kind.lhs_id == old_operand_id) {
          kind.lhs_id = new_operand_id;
        }
        if (kind.rhs_id == old_operand_id) {
          kind.rhs_id = new_operand_id;
        }
      },
      [&](instruction::ICmp& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        if (kind.lhs_id == old_operand_id) {
          kind.lhs_id = new_operand_id;
        }
        if (kind.rhs_id == old_operand_id) {
          kind.rhs_id = new_operand_id;
        }
      },
      [&](instruction::FCmp& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        if (kind.lhs_id == old_operand_id) {
          kind.lhs_id = new_operand_id;
        }
        if (kind.rhs_id == old_operand_id) {
          kind.rhs_id = new_operand_id;
        }
      },
      [&](instruction::Cast& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        if (kind.src_id == old_operand_id) {
          kind.src_id = new_operand_id;
        }
      },
      [&](instruction::Ret& kind) {
        if (kind.maybe_value_id.has_value() && kind.maybe_value_id.value() == old_operand_id) {
          kind.maybe_value_id = new_operand_id;
        }
      },
      [&](instruction::CondBr& kind) {
        if (kind.cond_id == old_operand_id) {
          kind.cond_id = new_operand_id;
        }
      },
      [&](instruction::Br& kind) {},
      [&](instruction::Phi& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        for (auto& [value_id, block_id] : kind.incoming_list) {
          if (value_id == old_operand_id) {
            value_id = new_operand_id;
          }
        }
      },
      [&](instruction::Alloca& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        if (kind.maybe_size_id.has_value() && kind.maybe_size_id.value() == old_operand_id) {
          kind.maybe_size_id = new_operand_id;
        }
        if (kind.maybe_align_id.has_value() && kind.maybe_align_id.value() == old_operand_id) {
          kind.maybe_align_id = new_operand_id;
        }
        if (kind.maybe_addrspace_id.has_value() && kind.maybe_addrspace_id.value() == old_operand_id) {
          kind.maybe_addrspace_id = new_operand_id;
        }
      },
      [&](instruction::Load& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        if (kind.ptr_id == old_operand_id) {
          kind.ptr_id = new_operand_id;
        }
      },
      [&](instruction::Store& kind) {
        if (kind.value_id == old_operand_id) {
          kind.value_id = new_operand_id;
        }
        if (kind.ptr_id == old_operand_id) {
          kind.ptr_id = new_operand_id;
        }
      },
      [&](instruction::Call& kind) {
        if (kind.maybe_dst_id.has_value() && kind.maybe_dst_id.value() == old_operand_id) {
          kind.maybe_dst_id = new_operand_id;
        }
        for (auto& arg_id : kind.arg_id_list) {
          if (arg_id == old_operand_id) {
            arg_id = new_operand_id;
          }
        }
      },
      [&](instruction::GetElementPtr& kind) {
        if (kind.dst_id == old_operand_id) {
          kind.dst_id = new_operand_id;
        }
        if (kind.ptr_id == old_operand_id) {
          kind.ptr_id = new_operand_id;
        }
        for (auto& index_id : kind.index_id_list) {
          if (index_id == old_operand_id) {
            index_id = new_operand_id;
          }
        }
      },
      [&](instruction::Dummy& kind) {},
    },
    this->kind
  );
}

void Instruction::remove(Context& context) {
  std::visit(
    overloaded{
      [this, &context](const instruction::Binary& kind) {
        context.operand_table[kind.lhs_id]->remove_use(this->id);
        context.operand_table[kind.rhs_id]->remove_use(this->id);
      },
      [this, &context](const instruction::ICmp& kind) {
        context.operand_table[kind.lhs_id]->remove_use(this->id);
        context.operand_table[kind.rhs_id]->remove_use(this->id);
      },
      [this, &context](const instruction::FCmp& kind) {
        context.operand_table[kind.lhs_id]->remove_use(this->id);
        context.operand_table[kind.rhs_id]->remove_use(this->id);
      },
      [this, &context](const instruction::Cast& kind) {
        context.operand_table[kind.src_id]->remove_use(this->id);
      },
      [this, &context](const instruction::Ret& kind) {
        if (kind.maybe_value_id.has_value()) {
          context.operand_table[kind.maybe_value_id.value()]->remove_use(
            this->id
          );
        }
      },
      [this, &context](const instruction::CondBr& kind) {
        context.operand_table[kind.cond_id]->remove_use(this->id);
        context.basic_block_table[kind.then_block_id]->remove_use(this->id);
        context.basic_block_table[kind.else_block_id]->remove_use(this->id);

        context.basic_block_table[kind.then_block_id]->remove_pred(
          this->parent_block_id
        );
        context.basic_block_table[kind.else_block_id]->remove_pred(
          this->parent_block_id
        );

        context.basic_block_table[this->parent_block_id]->remove_succ(
          kind.then_block_id
        );
        context.basic_block_table[this->parent_block_id]->remove_succ(
          kind.else_block_id
        );
      },
      [this, &context](const instruction::Br& kind) {
        context.basic_block_table[kind.block_id]->remove_use(this->id);

        context.basic_block_table[kind.block_id]->remove_pred(
          this->parent_block_id
        );
        context.basic_block_table[this->parent_block_id]->remove_succ(
          kind.block_id
        );
      },
      [this, &context](const instruction::Phi& kind) {
        for (auto [operand_id, basic_block_id] : kind.incoming_list) {
          context.operand_table[operand_id]->remove_use(this->id);
          context.basic_block_table[basic_block_id]->remove_use(this->id);
        }
      },
      [this, &context](const instruction::Alloca& kind) {
        if (kind.maybe_size_id.has_value()) {
          context.operand_table[kind.maybe_size_id.value()]->remove_use(this->id
          );
        }
        if (kind.maybe_align_id.has_value()) {
          context.operand_table[kind.maybe_align_id.value()]->remove_use(
            this->id
          );
        }
        if (kind.maybe_addrspace_id.has_value()) {
          context.operand_table[kind.maybe_addrspace_id.value()]->remove_use(
            this->id
          );
        }
      },
      [this, &context](const instruction::Load& kind) {
        context.operand_table[kind.ptr_id]->remove_use(this->id);
        if (kind.maybe_align_id.has_value()) {
          context.operand_table[kind.maybe_align_id.value()]->remove_use(
            this->id
          );
        }
      },
      [this, &context](const instruction::Store& kind) {
        context.operand_table[kind.value_id]->remove_use(this->id);
        context.operand_table[kind.ptr_id]->remove_use(this->id);
        if (kind.maybe_align_id.has_value()) {
          context.operand_table[kind.maybe_align_id.value()]->remove_use(
            this->id
          );
        }
      },
      [this, &context](const instruction::Call& kind) {
        if (kind.maybe_dst_id.has_value()) {
          context.operand_table[kind.maybe_dst_id.value()]->remove_use(this->id
          );
        }
        context.function_table[kind.function_name]->remove_caller(this->id);
        for (auto arg_id : kind.arg_id_list) {
          context.operand_table[arg_id]->remove_use(this->id);
        }
      },
      [this, &context](const instruction::GetElementPtr& kind) {
        context.operand_table[kind.ptr_id]->remove_use(this->id);
        for (auto index_id : kind.index_id_list) {
          context.operand_table[index_id]->remove_use(this->id);
        }
      },
      [this, &context](const instruction::Dummy& kind) {},
    },
    this->kind
  );

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

bool Instruction::is_terminator() const {
  return as<instruction::Br>().has_value() ||
         as<instruction::CondBr>().has_value() ||
         as<instruction::Ret>().has_value();
}

bool Instruction::is_alloca() const {
  return std::holds_alternative<instruction::Alloca>(this->kind);
}

bool Instruction::is_store() const {
  return std::holds_alternative<instruction::Store>(this->kind);
}

bool Instruction::is_load() const {
  return std::holds_alternative<instruction::Load>(this->kind);
}

bool Instruction::is_phi() const {
  return std::holds_alternative<instruction::Phi>(this->kind);
}

bool Instruction::is_br() const {
  return std::holds_alternative<instruction::Br>(this->kind);
}

bool Instruction::is_binary() const {
  return std::holds_alternative<instruction::Binary>(this->kind);
}

bool Instruction::is_call() const {
  return std::holds_alternative<instruction::Call>(this->kind);
}

bool Instruction::is_ret() const {
  return std::holds_alternative<instruction::Ret>(this->kind);
}

bool Instruction::is_getelementptr() const {
  return std::holds_alternative<instruction::GetElementPtr>(this->kind);
}

bool Instruction::is_icmp() const {
  return std::holds_alternative<instruction::ICmp>(this->kind);
}

bool Instruction::is_fcmp() const {
  return std::holds_alternative<instruction::FCmp>(this->kind);
}

bool Instruction::is_cast() const {
  return std::holds_alternative<instruction::Cast>(this->kind);
}

void Instruction::add_phi_operand(
  OperandID incoming_operand_id,
  BasicBlockID incoming_block_id,
  Context& context
) {
  if (!is_phi()) {
    return;
  }
  std::get<instruction::Phi>(this->kind)
    .incoming_list.emplace_back(incoming_operand_id, incoming_block_id);
  context.operand_table[incoming_operand_id]->add_use(this->id);
  context.basic_block_table[incoming_block_id]->add_use(this->id);
  this->add_use(incoming_operand_id);
}

std::optional<OperandID> Instruction::remove_phi_operand(
  BasicBlockID incoming_block_id,
  Context& context
) {
  if (!is_phi()) {
    return std::nullopt;
  }
  // Remove the use of block and operand and this instruction
  auto& incoming_list = std::get<instruction::Phi>(this->kind).incoming_list;
  auto it = std::find_if(
    incoming_list.begin(), incoming_list.end(),
    [incoming_block_id](const auto& pair) {
      return std::get<1>(pair) == incoming_block_id;
    }
  );

  if (it == incoming_list.end()) {
    return std::nullopt;
  }

  auto operand_id = std::get<0>(*it);
  context.operand_table[operand_id]->remove_use(this->id);
  context.basic_block_table[incoming_block_id]->remove_use(this->id);
  this->remove_use(operand_id);
  
  // Erase from the incoming list
  incoming_list.erase(
    std::remove(incoming_list.begin(), incoming_list.end(), *it),
    incoming_list.end()
  );


  return operand_id;
}

Instruction::Instruction(
  InstructionID id,
  InstructionKind kind,
  BasicBlockID parent_block_id
)
  : id(id), kind(kind), parent_block_id(parent_block_id), next(nullptr) {}

std::string Instruction::to_string(Context& context) {
  using namespace instruction;

  std::string result = "";

  result = std::visit(
    overloaded{
      [&context](const Binary& instruction) {
        auto dst_str = context.get_operand(instruction.dst_id)->to_string();

        // Type in the binary instruction operands must be the same
        auto type = context.get_operand(instruction.lhs_id)->get_type();

        auto lhs_str = context.get_operand(instruction.lhs_id)->to_string();
        auto rhs_str = context.get_operand(instruction.rhs_id)->to_string();

        std::string result = "";

        switch (instruction.op) {
          case BinaryOp::Add:
            result = dst_str + " = add " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::Sub:
            result = dst_str + " = sub " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::Mul:
            result = dst_str + " = mul " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::SDiv:
            result = dst_str + " = sdiv " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::SRem:
            result = dst_str + " = srem " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::FAdd:
            result = dst_str + " = fadd " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::FSub:
            result = dst_str + " = fsub " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::FMul:
            result = dst_str + " = fmul " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::FDiv:
            result = dst_str + " = fdiv " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::Shl:
            result = dst_str + " = shl " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::LShr:
            result = dst_str + " = lshr " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::AShr:
            result = dst_str + " = ashr " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::And:
            result = dst_str + " = and " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::Or:
            result = dst_str + " = or " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;
          case BinaryOp::Xor:
            result = dst_str + " = xor " + type->to_string() + " " + lhs_str +
                     ", " + rhs_str;
            break;

          default:
            // unreachable
            break;
        }
        return result;
      },
      [&context](const ICmp& instruction) {
        auto dst_str = context.get_operand(instruction.dst_id)->to_string();

        // Type in the icmp instruction operands must be the same
        auto type = context.get_operand(instruction.lhs_id)->get_type();

        auto lhs_str = context.get_operand(instruction.lhs_id)->to_string();
        auto rhs_str = context.get_operand(instruction.rhs_id)->to_string();

        std::string result = "";

        switch (instruction.cond) {
          case ICmpCond::Eq:
            result = dst_str + " = icmp eq " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case ICmpCond::Ne:
            result = dst_str + " = icmp ne " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case ICmpCond::Slt:
            result = dst_str + " = icmp slt " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case ICmpCond::Sle:
            result = dst_str + " = icmp sle " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          default:
            // unreachable
            break;
        }

        return result;
      },
      [&context](const FCmp& instruction) {
        auto dst_str = context.get_operand(instruction.dst_id)->to_string();

        // Type in the fcmp instruction operands must be the same
        auto type = context.get_operand(instruction.lhs_id)->get_type();

        auto lhs_str = context.get_operand(instruction.lhs_id)->to_string();
        auto rhs_str = context.get_operand(instruction.rhs_id)->to_string();

        std::string result = "";

        switch (instruction.cond) {
          case FCmpCond::Oeq:
            result = dst_str + " = fcmp oeq " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case FCmpCond::One:
            result = dst_str + " = fcmp one " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case FCmpCond::Olt:
            result = dst_str + " = fcmp olt " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case FCmpCond::Ole:
            result = dst_str + " = fcmp ole " + type->to_string() + " " +
                     lhs_str + ", " + rhs_str;
            break;
          default:
            // unreachable
            break;
        }

        return result;
      },
      [&context](const Cast& instruction) {
        auto dst = context.get_operand(instruction.dst_id);

        // Get the dst type from the dst operand.
        auto dst_type = dst->get_type();

        auto dst_str = dst->to_string();

        auto src = context.get_operand(instruction.src_id);
        auto src_type = src->get_type();
        auto src_str = src->to_string();

        std::string result = "";

        switch (instruction.op) {
          case CastOp::ZExt:
            result = dst_str + " = zext " + src_type->to_string() + " " +
                     src_str + " to " + dst_type->to_string();
            break;
          case CastOp::BitCast:
            result = dst_str + " = bitcast " + src_type->to_string() + " " +
                     src_str + " to " + dst_type->to_string();
            break;
          case CastOp::FPToSI:
            result = dst_str + " = fptosi " + src_type->to_string() + " " +
                     src_str + " to " + dst_type->to_string();
            break;
          case CastOp::SIToFP:
            result = dst_str + " = sitofp " + src_type->to_string() + " " +
                     src_str + " to " + dst_type->to_string();
            break;
          default:
            // unreachable
            break;
        }

        return result;
      },
      [&context](const Ret& instruction) -> std::string {
        if (instruction.maybe_value_id.has_value()) {
          auto value = context.get_operand(instruction.maybe_value_id.value());
          auto value_type = value->get_type();
          auto value_str = value->to_string();
          return "ret " + value_type->to_string() + " " + value_str;
        } else {
          return "ret void";
        }
      },
      [&context](const CondBr& instruction) {
        auto cond = context.get_operand(instruction.cond_id);

        auto cond_str = cond->to_string();
        auto cond_type = cond->get_type();

        auto true_block = context.get_basic_block(instruction.then_block_id);
        auto false_block = context.get_basic_block(instruction.else_block_id);

        auto true_block_label_str = "%" + true_block->get_label();
        auto false_block_label_str = "%" + false_block->get_label();

        return "br " + cond_type->to_string() + " " + cond_str + ", label " +
               true_block_label_str + ", label " + false_block_label_str;
      },
      [&context](const Br& instruction) {
        auto block = context.get_basic_block(instruction.block_id);
        auto block_label_str = "%" + block->get_label();
        return "br label " + block_label_str;
      },
      [&context](const Phi& instruction) {
        auto dst = context.get_operand(instruction.dst_id);
        auto dst_type = dst->get_type();
        auto dst_str = dst->to_string();

        std::string result = dst_str + " = phi " + dst_type->to_string() + " ";

        for (auto& [value_id, block_id] : instruction.incoming_list) {
          auto value = context.get_operand(value_id);
          auto value_type = value->get_type();
          auto value_str = value->to_string();

          auto block = context.get_basic_block(block_id);
          auto block_label_str = "%" + block->get_label();

          result += "[" + value_str + ", " + block_label_str + "], ";
        }

        if (instruction.incoming_list.size() > 0) {
          result.pop_back();
          result.pop_back();
        }

        return result;
      },
      [&context](const Alloca& instruction) {
        auto dst_str = context.get_operand(instruction.dst_id)->to_string();
        auto allocated_type_str = instruction.allocated_type->to_string();
        auto result = dst_str + " = alloca " + allocated_type_str;

        if (instruction.maybe_size_id.has_value()) {
          auto size = context.get_operand(instruction.maybe_size_id.value());
          auto size_type_str = size->get_type()->to_string();
          auto size_str = size->to_string();

          result += ", " + size_type_str + " " + size_str;
        }

        if (instruction.maybe_align_id.has_value()) {
          auto align_str =
            context.get_operand(instruction.maybe_align_id.value())
              ->to_string();
          result += ", align " + align_str;
        }

        if (instruction.maybe_addrspace_id.has_value()) {
          auto addrspace_str =
            context.get_operand(instruction.maybe_addrspace_id.value())
              ->to_string();
          result += ", addrspace(" + addrspace_str + ")";
        }

        return result;
      },
      [&context](const Load& instruction) {
        auto dst = context.get_operand(instruction.dst_id);
        auto dst_type_str = dst->get_type()->to_string();
        auto dst_str = dst->to_string();

        auto ptr = context.get_operand(instruction.ptr_id);
        auto ptr_type_str = ptr->get_type()->to_string();
        auto ptr_str = ptr->to_string();

        auto result = dst_str + " = load " + dst_type_str + ", " +
                      ptr_type_str + " " + ptr_str;

        if (instruction.maybe_align_id.has_value()) {
          auto align_str =
            context.get_operand(instruction.maybe_align_id.value())
              ->to_string();
          result += ", align " + align_str;
        }

        return result;
      },
      [&context](const Store& instruction) {
        auto value = context.get_operand(instruction.value_id);
        auto value_type_str = value->get_type()->to_string();
        auto value_str = value->to_string();

        auto ptr = context.get_operand(instruction.ptr_id);
        auto ptr_type_str = ptr->get_type()->to_string();
        auto ptr_str = ptr->to_string();

        auto result = "store " + value_type_str + " " + value_str + ", " +
                      ptr_type_str + " " + ptr_str;

        if (instruction.maybe_align_id.has_value()) {
          auto align_str =
            context.get_operand(instruction.maybe_align_id.value())
              ->to_string();
          result += ", align " + align_str;
        }

        return result;
      },
      [&context](const Call& instruction) {
        auto function = context.get_function(instruction.function_name);

        auto return_type_str = function->return_type->to_string();

        auto result =
          "call " + return_type_str + " @" + instruction.function_name + "(";

        for (auto& operand_id : instruction.arg_id_list) {
          auto operand = context.get_operand(operand_id);
          auto operand_type_str = operand->get_type()->to_string();
          auto operand_str = operand->to_string();

          result += operand_type_str + " " + operand_str + ", ";
        }

        if (instruction.arg_id_list.size() > 0) {
          result.pop_back();
          result.pop_back();
        }

        result += ")";

        if (instruction.maybe_dst_id.has_value()) {
          auto dst_str =
            context.get_operand(instruction.maybe_dst_id.value())->to_string();
          result = dst_str + " = " + result;
        }

        return result;
      },
      [&context](const GetElementPtr& instruction) {
        auto dst_str = context.get_operand(instruction.dst_id)->to_string();

        auto basis_type_str = instruction.basis_type->to_string();

        auto ptr = context.get_operand(instruction.ptr_id);
        auto ptr_type_str = ptr->get_type()->to_string();
        auto ptr_str = ptr->to_string();

        auto result = dst_str + " = getelementptr " + basis_type_str + ", " +
                      ptr_type_str + " " + ptr_str;

        for (auto& operand_id : instruction.index_id_list) {
          auto operand = context.get_operand(operand_id);
          auto operand_type_str = operand->get_type()->to_string();
          auto operand_str = operand->to_string();

          result += ", " + operand_type_str + " " + operand_str;
        }

        return result;
      },
      [&context](const auto& instruction) -> std::string { return ""; },
    },
    kind
  );

  result += "\t; parent: " + std::to_string(this->parent_block_id);

  return result;
}

}  // namespace ir
}  // namespace syc