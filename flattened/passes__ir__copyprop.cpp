#include "passes__ir__copyprop.h"

namespace syc {
namespace ir {

void copyprop(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    copyprop_function(function, builder);
  }
}

void copyprop_function(FunctionPtr function, Builder& builder) {
  using namespace instruction;

  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    auto curr_instr = curr_bb->head_instruction->next;

    while (curr_instr != curr_bb->tail_instruction) {
      auto next_instr = curr_instr->next;

      auto maybe_binary = curr_instr->as<Binary>();

      if (maybe_binary.has_value()) {
        auto binary = maybe_binary.value();
        auto op = binary.op;
        auto dst = builder.context.get_operand(binary.dst_id);
        auto lhs = builder.context.get_operand(binary.lhs_id);
        auto rhs = builder.context.get_operand(binary.rhs_id);

        if (lhs->is_constant() && rhs->is_constant()) {
          auto lhs_constant = std::get<operand::ConstantPtr>(lhs->kind);
          auto rhs_constant = std::get<operand::ConstantPtr>(rhs->kind);

          OperandID new_dst_id = dst->id;

          if (dst->type->as<type::Float>().has_value()) {
            // TODO: copyprop for float
          } else if (dst->type->as<type::Integer>().has_value()) {
            auto lhs_int = std::get<int>(lhs_constant->kind);
            auto rhs_int = std::get<int>(rhs_constant->kind);

            int result_int;

            switch (op) {
              case BinaryOp::Add: {
                result_int = lhs_int + rhs_int;
                new_dst_id =
                  builder.fetch_constant_operand(dst->type, result_int);
                break;
              }
              case BinaryOp::Sub: {
                result_int = lhs_int - rhs_int;
                new_dst_id =
                  builder.fetch_constant_operand(dst->type, result_int);
                break;
              }
              case BinaryOp::Mul: {
                result_int = lhs_int * rhs_int;
                new_dst_id =
                  builder.fetch_constant_operand(dst->type, result_int);
                break;
              }
              case BinaryOp::SDiv: {
                result_int = lhs_int / rhs_int;
                new_dst_id =
                  builder.fetch_constant_operand(dst->type, result_int);
                break;
              }
              case BinaryOp::SRem: {
                result_int = lhs_int % rhs_int;
                new_dst_id =
                  builder.fetch_constant_operand(dst->type, result_int);
                break;
              }
              case BinaryOp::Shl: {
                result_int = lhs_int << rhs_int;
                new_dst_id =
                  builder.fetch_constant_operand(dst->type, result_int);
                break;
              }
              default: {
                break;
              }
            }
          }

          if (new_dst_id != dst->id) {
            // Replace all uses of dst with new_dst_id
            auto use_id_list_copy = dst->use_id_list;
            for (auto use_id : use_id_list_copy) {
              auto use_instr = builder.context.get_instruction(use_id);
              use_instr->replace_operand(dst->id, new_dst_id, builder.context);
            }

            // Remove the instruction
            curr_instr->remove(builder.context);
          }
        }
      }

      curr_instr = next_instr;
    }

    curr_bb = curr_bb->next;
  }
}

}  // namespace ir
}  // namespace syc