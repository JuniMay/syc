#include "ir/instruction.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/function.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {
namespace ir {

std::string Instruction::to_string(Context& context) {
  using namespace instruction;

  return std::visit(
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
            result = dst_str + " = add " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case BinaryOp::Sub:
            result = dst_str + " = sub " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case BinaryOp::Mul:
            result = dst_str + " = mul " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case BinaryOp::SDiv:
            result = dst_str + " = sdiv " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;

          case BinaryOp::FAdd:
            result = dst_str + " = fadd " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case BinaryOp::FSub:
            result = dst_str + " = fsub " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case BinaryOp::FMul:
            result = dst_str + " = fmul " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case BinaryOp::FDiv:
            result = dst_str + " = fdiv " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
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
            result = dst_str + " = icmp eq " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case ICmpCond::Ne:
            result = dst_str + " = icmp ne " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case ICmpCond::Slt:
            result = dst_str + " = icmp slt " + type::to_string(type) + " " +
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
            result = dst_str + " = fcmp oeq " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case FCmpCond::Olt:
            result = dst_str + " = fcmp olt " + type::to_string(type) + " " +
                     lhs_str + ", " + rhs_str;
            break;
          case FCmpCond::Ole:
            result = dst_str + " = fcmp ole " + type::to_string(type) + " " +
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
            result = dst_str + " = zext " + type::to_string(src_type) + " " +
                     src_str + " to " + type::to_string(dst_type);
            break;
          case CastOp::BitCast:
            result = dst_str + " = bitcast " + type::to_string(src_type) + " " +
                     src_str + " to " + type::to_string(dst_type);
            break;
          case CastOp::FPToSI:
            result = dst_str + " = fptosi " + type::to_string(src_type) + " " +
                     src_str + " to " + type::to_string(dst_type);
            break;
          case CastOp::SIToFP:
            result = dst_str + " = sitofp " + type::to_string(src_type) + " " +
                     src_str + " to " + type::to_string(dst_type);
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
          return "ret " + type::to_string(value_type) + " " + value_str;
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

        return "br " + type::to_string(cond_type) + " " + cond_str +
               ", label " + true_block_label_str + ", label " +
               false_block_label_str;
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

        std::string result =
          dst_str + " = phi " + type::to_string(dst_type) + " ";

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
        auto allocated_type_str = type::to_string(instruction.allocated_type);
        auto result = dst_str + " = alloca " + allocated_type_str;

        if (instruction.maybe_size_id.has_value()) {
          auto size = context.get_operand(instruction.maybe_size_id.value());
          auto size_type_str = type::to_string(size->get_type());
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
        auto dst_type_str = type::to_string(dst->get_type());
        auto dst_str = dst->to_string();

        auto ptr = context.get_operand(instruction.ptr_id);
        auto ptr_type_str = type::to_string(ptr->get_type());
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
        auto value_type_str = type::to_string(value->get_type());
        auto value_str = value->to_string();

        auto ptr = context.get_operand(instruction.ptr_id);
        auto ptr_type_str = type::to_string(ptr->get_type());
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

        auto return_type_str = type::to_string(function->return_type);

        auto result =
          "call " + return_type_str + " @" + instruction.function_name + "(";

        for (auto& operand_id : instruction.arg_id_list) {
          auto operand = context.get_operand(operand_id);
          auto operand_type_str = type::to_string(operand->get_type());
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

        auto basis_type_str = type::to_string(instruction.basis_type);

        auto ptr = context.get_operand(instruction.ptr_id);
        auto ptr_type_str = type::to_string(ptr->get_type());
        auto ptr_str = ptr->to_string();

        auto result = dst_str + " = getelementptr " + basis_type_str + ", " +
                      ptr_type_str + " " + ptr_str;

        for (auto& operand_id : instruction.index_id_list) {
          auto operand = context.get_operand(operand_id);
          auto operand_type_str = type::to_string(operand->get_type());
          auto operand_str = operand->to_string();

          result += ", " + operand_type_str + " " + operand_str;
        }

        return result;
      }},

    kind
  );
}

}  // namespace ir
}  // namespace syc