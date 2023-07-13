#include "passes/ir_peephole.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void peephole(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    peephole_function(function, builder);
  }
}

void peephole_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    peephole_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void peephole_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    if (std::holds_alternative<instruction::Binary>(curr_instruction->kind)) {
        const auto& kind = std::get<instruction::Binary>(curr_instruction->kind);
        auto op = kind.op;
        auto dst = builder.context.get_operand(kind.dst_id);
        auto lhs = builder.context.get_operand(kind.lhs_id);
        auto rhs = builder.context.get_operand(kind.rhs_id);

        if (op == instruction::BinaryOp::Mul 
            && lhs->is_int()
            && rhs->is_int()
            && rhs->is_constant()) {
            auto constant = std::get<operand::ConstantPtr>(rhs->kind);
            auto constant_value = std::get<int>(constant->kind);
            if (constant_value == 0) {
                // mul lhs, 0 -> make all uses of dst to 0
                auto use_id_list_copy = dst->use_id_list;
                auto zero = builder.fetch_constant_operand(builder.fetch_i32_type(), (int)0);
                for (auto use_instruction_id : use_id_list_copy) {
                    auto instruction = builder.context.get_instruction(use_instruction_id);
                    instruction->replace_operand(dst->id, zero, builder.context);
                }
                curr_instruction->remove(builder.context);
            } else if (constant_value > 1 && (constant_value & (constant_value - 1)) == 0) {
                // dest = mul lhs, 2^k -> dest = shl lhs, k
                curr_instruction->insert_next(builder.fetch_binary_instruction(
                    instruction::BinaryOp::Shl,
                    dst->id,
                    lhs->id,
                    builder.fetch_constant_operand(builder.fetch_i32_type(), (int)log2(constant_value))
                ));
                curr_instruction->remove(builder.context);
            }
            // TODO: maybe optimize mul lhs, 2^k + 1

        } else if (op == instruction::BinaryOp::SDiv
            && lhs->is_int()
            && rhs->is_int()
            && rhs->is_constant()) {
          auto constant = std::get<operand::ConstantPtr>(rhs->kind);
          auto constant_value = std::get<int>(constant->kind);
          if (constant_value == 1) {
              // div lhs 1, make all uses of dst to lhs
              auto use_id_list_copy = dst->use_id_list;
              std::cout << "replace " << dst->id << " with " << lhs << std::endl;
              for (auto use_instruction_id : use_id_list_copy) {
                  std::cout << "use instruction id: " << use_instruction_id << std::endl;
                  auto instruction = builder.context.get_instruction(use_instruction_id);
                  instruction->replace_operand(dst->id, lhs->id, builder.context);
              }
              curr_instruction->remove(builder.context);
          } else if (constant_value == -1) {
            // div lhs, -1 -> dest = sub 0, lhs
            curr_instruction->insert_next(builder.fetch_binary_instruction(
                instruction::BinaryOp::Sub,
                dst->id,
                builder.fetch_constant_operand(builder.fetch_i32_type(), (int)0),
                lhs->id
            ));
            curr_instruction->remove(builder.context);
          } else if (constant_value == 2) {
              // dest = div lhs, 2
              // -> 
              // tmp = srl lhs, 31
              // dest = add tmp, lhs
              // dest = sra dest, 1
              auto tmp0 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
              auto tmp1 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
              curr_instruction->insert_next(builder.fetch_binary_instruction(
                  instruction::BinaryOp::AShr,
                  dst->id,
                  tmp1,
                  builder.fetch_constant_operand(builder.fetch_i32_type(), (int)1)
              ));
              curr_instruction->insert_next(builder.fetch_binary_instruction(
                  instruction::BinaryOp::Add,
                  tmp1,
                  tmp0,
                  lhs->id
              ));
              curr_instruction->insert_next(builder.fetch_binary_instruction(
                  instruction::BinaryOp::LShr,
                  tmp0,
                  lhs->id,
                  builder.fetch_constant_operand(builder.fetch_i32_type(), (int)31)
              ));
              curr_instruction->remove(builder.context);
          }
        }
    }
    curr_instruction = next_instruction;
  }
}
    
}  // namespace ir
}  // namespace syc