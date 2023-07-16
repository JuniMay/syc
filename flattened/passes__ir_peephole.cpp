#include "passes__ir_peephole.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

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
      const auto& curr_kind =
        std::get<instruction::Binary>(curr_instruction->kind);
      auto curr_op = curr_kind.op;
      auto curr_dst = builder.context.get_operand(curr_kind.dst_id);
      auto curr_lhs = builder.context.get_operand(curr_kind.lhs_id);
      auto curr_rhs = builder.context.get_operand(curr_kind.rhs_id);

      if (curr_op == instruction::BinaryOp::Mul 
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value == 0) {
          // mul lhs, 0 -> make all uses of dst to 0
          auto use_id_list_copy = curr_dst->use_id_list;
          auto zero =
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)0);
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(curr_dst->id, zero, builder.context);
          }
          curr_instruction->remove(builder.context);
        } else if (constant_value > 1 && (constant_value & (constant_value - 1)) == 0) {
          // dest = mul lhs, 2^k -> dest = shl lhs, k
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Shl, curr_dst->id, curr_lhs->id,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)log2(constant_value)
            )
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        }
        // TODO: maybe optimize mul lhs, 2^k + 1

      } else if (curr_op == instruction::BinaryOp::SDiv
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value == 1) {
          // div lhs 1, make all uses of dst to lhs
          auto use_id_list_copy = curr_dst->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(
              curr_dst->id, curr_lhs->id, builder.context
            );
          }
          curr_instruction->remove(builder.context);
        } else if (constant_value == -1) {
          // div lhs, -1 -> dest = sub 0, lhs
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Sub, curr_dst->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)0),
            curr_lhs->id
          ));
          next_instruction = curr_instruction->next;
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
            instruction::BinaryOp::AShr, curr_dst->id, tmp1,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)1)
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Add, tmp1, tmp0, curr_lhs->id
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::LShr, tmp0, curr_lhs->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)31)
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        } else if (constant_value > 2 && (constant_value & (constant_value - 1)) == 0) {
          // dest = div lhs, 2^k
          // ->
          // t0 = sra lhs, 31
          // t1 = srl t0, 32 - k
          // t2 = add lhs, t1
          // dest = sra t2, k
          auto tmp0 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp1 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::AShr, curr_dst->id, tmp1,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)constant_value
            )
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Add, tmp1, tmp0, curr_lhs->id
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::LShr, tmp0, curr_lhs->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)31)
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        }
      } 
      else if (curr_op == instruction::BinaryOp::SRem
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        // dest = srem lhs, 2^k
        // ->
        // t0 = sra lhs, 31
        // t1 = srl t0, 32 - k
        // t2 = add lhs, t1
        // t3 = and t2, 2^k - 1
        // dest = sub t3, t1
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value > 1 && (constant_value & (constant_value - 1)) == 0) {
          auto tmp0 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp1 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp2 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto tmp3 = builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Sub, curr_dst->id, tmp3, tmp1
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::And, tmp3, tmp2,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)constant_value - 1
            )
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Add, tmp2, tmp1, curr_lhs->id
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::LShr, tmp1, tmp0,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)(32 - log2(constant_value))
            )
          ));
          curr_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::AShr, tmp0, curr_lhs->id,
            builder.fetch_constant_operand(builder.fetch_i32_type(), (int)31)
          ));
          next_instruction = curr_instruction->next;
          curr_instruction->remove(builder.context);
        }
      } 
      else if (curr_op == instruction::BinaryOp::Sub
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        // dest = sub lhs, c
        // ->
        // dest = add lhs, -c
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        curr_instruction->insert_next(builder.fetch_binary_instruction(
          instruction::BinaryOp::Add, curr_dst->id, curr_lhs->id,
          builder.fetch_constant_operand(
            builder.fetch_i32_type(), (int)-constant_value
          )
        ));
        next_instruction = curr_instruction->next;
        curr_instruction->remove(builder.context);
      } else if (curr_op == instruction::BinaryOp::Add
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        // dest = add lhs, 0
        // ->
        // make all uses of dest to lhs
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value == 0) {
          auto use_id_list_copy = curr_dst->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(
              curr_dst->id, curr_lhs->id, builder.context
            );
          }
          curr_instruction->remove(builder.context);
        }
      }
      if (std::holds_alternative<instruction::Binary>(next_instruction->kind)) {
        const auto& next_kind =
          std::get<instruction::Binary>(next_instruction->kind);
        auto next_op = next_kind.op;
        auto next_dst = builder.context.get_operand(next_kind.dst_id);
        auto next_lhs = builder.context.get_operand(next_kind.lhs_id);
        auto next_rhs = builder.context.get_operand(next_kind.rhs_id);

        if (curr_op == instruction::BinaryOp::Add
          && curr_lhs->is_int() 
          && curr_rhs->is_int()
          && curr_rhs->is_constant()
          && next_op == instruction::BinaryOp::Add
          && next_lhs->is_int()
          && next_rhs->is_int()
          && next_rhs->is_constant()
          && curr_dst->id == next_lhs->id
          && curr_dst->use_id_list.size() == 1) {
          // dest1 = add lhs, c1
          // dest2 = add dest1, c2
          // -> dest2 = add lhs, c1 + c2
          auto constant0 = std::get<operand::ConstantPtr>(curr_rhs->kind);
          auto constant1 = std::get<operand::ConstantPtr>(next_rhs->kind);
          auto constant_value0 = std::get<int>(constant0->kind);
          auto constant_value1 = std::get<int>(constant1->kind);
          auto instruction = builder.fetch_binary_instruction(
            instruction::BinaryOp::Add, next_dst->id, curr_lhs->id,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)(constant_value0 + constant_value1)
            )
          );
          next_instruction->insert_next(instruction);

          curr_instruction->remove(builder.context);
          curr_instruction = next_instruction;
          next_instruction = next_instruction->next;
          curr_instruction->remove(builder.context);
        } else if (curr_op == instruction::BinaryOp::Sub
          && curr_lhs->is_int()
          && curr_rhs->is_int()
          && curr_rhs->is_constant()
          && next_op == instruction::BinaryOp::Sub
          && next_lhs->is_int()
          && next_rhs->is_int()
          && next_rhs->is_constant()
          && curr_dst->id == next_lhs->id
          && curr_dst->use_id_list.size() == 1) {
          // dest1 = sub lhs, c1
          // dest2 = sub dest1, c2
          // -> dest2 = sub lhs, c1 + c2
          auto constant0 = std::get<operand::ConstantPtr>(curr_rhs->kind);
          auto constant1 = std::get<operand::ConstantPtr>(next_rhs->kind);
          auto constant_value0 = std::get<int>(constant0->kind);
          auto constant_value1 = std::get<int>(constant1->kind);
          next_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Sub, next_dst->id, curr_lhs->id,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)(constant_value0 + constant_value1)
            )
          ));
          curr_instruction->remove(builder.context);
          curr_instruction = next_instruction;
          next_instruction = next_instruction->next;
          curr_instruction->remove(builder.context);
        } else if (curr_op == instruction::BinaryOp::Add
          && curr_lhs->is_int()
          && curr_rhs->is_int()
          && curr_rhs->is_constant()
          && next_op == instruction::BinaryOp::Sub
          && next_lhs->is_int()
          && next_rhs->is_int()
          && next_rhs->is_constant()
          && curr_dst->id == next_lhs->id
          && curr_dst->use_id_list.size() == 1) {
          // dest1 = add lhs, c1
          // dest2 = sub dest1, c2
          // -> dest2 = add lhs, c1 - c2
          auto constant0 = std::get<operand::ConstantPtr>(curr_rhs->kind);
          auto constant1 = std::get<operand::ConstantPtr>(next_rhs->kind);
          auto constant_value0 = std::get<int>(constant0->kind);
          auto constant_value1 = std::get<int>(constant1->kind);
          next_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Add, next_dst->id, curr_lhs->id,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)(constant_value0 - constant_value1)
            )
          ));
          curr_instruction->remove(builder.context);
          curr_instruction = next_instruction;
          next_instruction = next_instruction->next;
          curr_instruction->remove(builder.context);
        } else if (curr_op == instruction::BinaryOp::Sub
          && curr_lhs->is_int()
          && curr_rhs->is_int()
          && curr_rhs->is_constant()
          && next_op == instruction::BinaryOp::Add
          && next_lhs->is_int()
          && next_rhs->is_int()
          && next_rhs->is_constant()
          && curr_dst->id == next_lhs->id
          && curr_dst->use_id_list.size() == 1) {
          // dest1 = sub lhs, c1
          // dest2 = add dest1, c2
          // -> dest2 = add lhs, c2 - c1
          auto constant0 = std::get<operand::ConstantPtr>(curr_rhs->kind);
          auto constant1 = std::get<operand::ConstantPtr>(next_rhs->kind);
          auto constant_value0 = std::get<int>(constant0->kind);
          auto constant_value1 = std::get<int>(constant1->kind);
          next_instruction->insert_next(builder.fetch_binary_instruction(
            instruction::BinaryOp::Add, next_dst->id, curr_lhs->id,
            builder.fetch_constant_operand(
              builder.fetch_i32_type(), (int)(constant_value1 - constant_value0)
            )
          ));
          curr_instruction->remove(builder.context);
          curr_instruction = next_instruction;
          next_instruction = next_instruction->next;
          curr_instruction->remove(builder.context);
        }
      }
    }
    // TODO: remove getelementptr type ptr, 0, 0
    // else if (std::holds_alternative<instruction::GetElementPtr>) {
    //   auto curr_kind =
    //   std::get<instruction::GetElementPtr>(curr_instruction->kind); auto
    //   curr_dst = curr_kind.dst_id; auto curr_ptr = curr_kind.ptr_id; for
    //   (auto &curr_operand : curr_kind.index_id_list) {
    //     if (curr_operand.is_constant()) {
    //       auto constant = std::get<operand::ConstantPtr>(curr_operand->kind);
    //       auto constant_value = std::get<int>(constant->kind);
    //       if (constant_value == 0) {
    //         curr_instruction->insert_next(builder.fetch_binary_instruction(
    //             instruction::BinaryOp::Add,
    //             curr_dst,
    //             curr_ptr,
    //             curr_operand->id
    //         ));
    //         curr_instruction->remove(builder.context);
    //       }
    //     }
    //   }
    // }

    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc