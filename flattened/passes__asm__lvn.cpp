#include "passes__asm__lvn.h"
#include "backend__basic_block.h"
#include "backend__instruction.h"
#include "backend__operand.h"

namespace syc {
namespace backend {

void lvn(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    lvn_function(function, builder);
  }
}

void lvn_function(FunctionPtr function, Builder& builder) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    lvn_basic_block(curr_bb, builder);
    curr_bb = curr_bb->next;
  }
}

using LiExprMap = std::map<std::variant<int64_t, OperandID>, OperandID>; // <imm, reg>
using BinaryExprMap = std::map<std::tuple<instruction::Binary::Op, OperandID, OperandID>, OperandID>; // <op, lhs, rhs, reg>
using BinaryImmExprMap = std::map<std::tuple<instruction::BinaryImm::Op, OperandID, int32_t>, OperandID>; // <op, lhs, rhs, reg>

void lvn_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;
  LiExprMap li_expr_map;
  BinaryExprMap binary_expr_map;
  BinaryImmExprMap binary_imm_expr_map;

  auto curr_instr = basic_block->head_instruction->next;
  while (curr_instr != basic_block->tail_instruction) {
    auto next_instr = curr_instr->next;

    if (curr_instr->is_li()) {
      auto li_rd_id = curr_instr->as<Li>()->rd_id;
      auto li_rd = builder.context.get_operand(li_rd_id);
      
      if (!li_rd->is_vreg()) {
        curr_instr = curr_instr->next;
        continue;
      }

      if (li_rd->use_id_list.size() == 0) {
        curr_instr->remove(builder.context);
        std::cout << "remove unused li" << std::endl;
      }

      if (li_rd->def_id_list.size() > 1) {
        curr_instr = curr_instr->next;
        continue;
      }

      auto li_imm_id = curr_instr->as<Li>()->imm_id;
      auto li_imm = builder.context.get_operand(li_imm_id);
      std::variant<int64_t, OperandID> imm;
      if (li_imm->is_immediate()) {
        if (std::holds_alternative<int32_t>(std::get<Immediate>(li_imm->kind).value)) {
          imm = (int64_t)std::get<int32_t>(std::get<Immediate>(li_imm->kind).value);
        } else {
          imm = (int64_t)std::get<uint32_t>(std::get<Immediate>(li_imm->kind).value);
        }
      } else {
        imm = li_imm_id;
      }
      if (li_expr_map.count(imm) > 0) {
        // redundant li
        auto new_li_rd_id = li_expr_map[imm];
        for (InstructionID use_id : li_rd->use_id_list) {
          auto use_inst = builder.context.get_instruction(use_id);
          use_inst->replace_use_operand(li_rd_id, new_li_rd_id, builder.context);
        }
        curr_instr->remove(builder.context);
        std::cout << "remove redundant li" << std::endl;
      } else {
        li_expr_map[imm] = li_rd_id;
      }
    } else if (curr_instr->is_binary()) {
      auto binary_rd_id = curr_instr->as<Binary>()->rd_id;
      auto binary_rd = builder.context.get_operand(binary_rd_id);

      if (binary_rd->def_id_list.size() > 1) {
        curr_instr = curr_instr->next;
        continue;
      }

      auto binary_rs1_id = curr_instr->as<Binary>()->rs1_id;
      auto binary_rs1 = builder.context.get_operand(binary_rs1_id);
      auto binary_rs2_id = curr_instr->as<Binary>()->rs2_id;
      auto binary_rs2 = builder.context.get_operand(binary_rs2_id);
      auto binary_op = curr_instr->as<Binary>()->op;

      if (!binary_rd->is_vreg() || !binary_rs1->is_vreg() || !binary_rs2->is_vreg()) {
        curr_instr = curr_instr->next;
        continue;
      }

      // sort rs1 and rs2 asc if op is commutative
      if (binary_op == instruction::Binary::Op::ADD || binary_op == instruction::Binary::Op::MUL) {
        if (binary_rs1_id > binary_rs2_id) {
          std::swap(binary_rs1_id, binary_rs2_id);
          std::swap(binary_rs1, binary_rs2);
        }
      }
      auto binary_expr = std::make_tuple(binary_op, binary_rs1_id, binary_rs2_id);

      if (binary_expr_map.count(binary_expr) > 0) {
        // redundant binary
        auto new_binary_rd_id = binary_expr_map[binary_expr];
        for (InstructionID use_id : binary_rd->use_id_list) {
          auto use_inst = builder.context.get_instruction(use_id);
          use_inst->replace_use_operand(binary_rd_id, new_binary_rd_id, builder.context);
        }
        curr_instr->remove(builder.context);
        std::cout << "remove redundant binary" << std::endl;
      } else {
        binary_expr_map[binary_expr] = binary_rd_id;
      }
    } else if (curr_instr->is_binary_imm()) {
      auto binary_imm_rd_id = curr_instr->as<BinaryImm>()->rd_id;
      auto binary_imm_rd = builder.context.get_operand(binary_imm_rd_id);

      if (binary_imm_rd->def_id_list.size() > 1) {
        curr_instr = curr_instr->next;
        continue;
      }

      auto binary_imm_rs_id = curr_instr->as<BinaryImm>()->rs_id;
      auto binary_imm_rs = builder.context.get_operand(binary_imm_rs_id);
      auto binary_imm_imm_id = curr_instr->as<BinaryImm>()->imm_id;
      auto binary_imm_imm = builder.context.get_operand(binary_imm_imm_id);
      auto binary_imm_val = std::get<int32_t>(std::get<Immediate>(binary_imm_imm->kind).value);
      auto binary_imm_op = curr_instr->as<BinaryImm>()->op;

      if (!binary_imm_rd->is_vreg() || !binary_imm_rs->is_vreg()) {
        curr_instr = curr_instr->next;
        continue;
      }

      auto binary_imm_expr = std::make_tuple(binary_imm_op, binary_imm_rs_id, binary_imm_val);

      if (binary_imm_expr_map.count(binary_imm_expr) > 0) {
        // redundant binary_imm
        auto new_binary_imm_rd_id = binary_imm_expr_map[binary_imm_expr];
        for (InstructionID use_id : binary_imm_rd->use_id_list) {
          auto use_inst = builder.context.get_instruction(use_id);
          use_inst->replace_use_operand(binary_imm_rd_id, new_binary_imm_rd_id, builder.context);
        }
        curr_instr->remove(builder.context);
        std::cout << "remove redundant binary_imm" << std::endl;
      } else {
        binary_imm_expr_map[binary_imm_expr] = binary_imm_rd_id;
      }
    }
    curr_instr = next_instr;
  }
}

}  // namespace backend

}  // namespace syc