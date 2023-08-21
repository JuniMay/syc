#include "passes/asm/unused_store_elim.h"
#include "backend/builder.h"
#include "backend/context.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"

namespace syc {
namespace backend {

void unused_store_elim(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    unused_store_elim_function(function, builder);
  }
}

void unused_store_elim_function(FunctionPtr function, Builder& builder) {
  std::cout << "unused store elim" << std::endl;
  using namespace instruction;
  auto curr_bb = function->head_basic_block->next;
  std::vector<int> load_list;
  while (curr_bb != function->tail_basic_block) {
    auto curr_instr = curr_bb->head_instruction->next;
    while (curr_instr != curr_bb->tail_instruction) {
      auto next_instr = curr_instr->next;
      if (curr_instr->is_load()) {
        std::cout << "load" << std::endl;
        auto load_op = curr_instr->as<Load>()->op;
        auto load_rd_id = curr_instr->as<Load>()->rd_id;
        auto load_rd = builder.context.get_operand(load_rd_id);
        auto load_rs_id = curr_instr->as<Load>()->rs_id;
        auto load_rs = builder.context.get_operand(load_rs_id);
        auto load_imm_id = curr_instr->as<Load>()->imm_id;
        auto load_imm = builder.context.get_operand(load_imm_id);
        auto load_imm_val = std::get<int32_t>(std::get<Immediate>(load_imm->kind).value);

        if (load_op == Load::Op::LW && load_rs->is_sp()) {
          std::cout << "load sp" << std::endl;
          load_list.push_back(load_imm_val);
        }
      } else if (curr_instr->is_binary_imm()) {
        auto binary_rs_id = curr_instr->as<BinaryImm>()->rs_id;
        auto binary_rs = builder.context.get_operand(binary_rs_id);
        auto binary_rd_id = curr_instr->as<BinaryImm>()->rd_id;
        auto binary_rd = builder.context.get_operand(binary_rd_id);
        if (binary_rs->is_sp() && !binary_rd->is_sp()) {
          // no more optimization
          return;
        }
      } else if (curr_instr->is_binary()) {
        auto binary_rs1_id = curr_instr->as<Binary>()->rs1_id;
        auto binary_rs1 = builder.context.get_operand(binary_rs1_id);
        auto binary_rs2_id = curr_instr->as<Binary>()->rs2_id;
        auto binary_rs2 = builder.context.get_operand(binary_rs2_id);
        if (binary_rs1->is_sp() || binary_rs2->is_sp()) {
          // no more optimization
          return;
        }
      }
      curr_instr = next_instr;
    }
    curr_bb = curr_bb->next;
  }

  curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    auto curr_instr = curr_bb->head_instruction->next;
    while (curr_instr != curr_bb->tail_instruction) {
      auto next_instr = curr_instr->next;
      if (curr_instr->is_store()) {
        auto store_op = curr_instr->as<Store>()->op;
        auto store_rs1_id = curr_instr->as<Store>()->rs1_id;
        auto store_rs1 = builder.context.get_operand(store_rs1_id);
        auto store_rs2_id = curr_instr->as<Store>()->rs2_id;
        auto store_rs2 = builder.context.get_operand(store_rs2_id);
        auto store_imm_id = curr_instr->as<Store>()->imm_id;
        auto store_imm = builder.context.get_operand(store_imm_id);
        auto store_imm_val = std::get<int32_t>(std::get<Immediate>(store_imm->kind).value);

        if (store_op == Store::Op::SW && store_rs1->is_sp()) {
          std::cout << "store sp" << std::endl;
          if (std::find(load_list.begin(), load_list.end(), store_imm_val) != load_list.end()) {
            std::cout << "found" << std::endl;
          } else {
            std::cout << "not found" << std::endl;
            curr_instr->remove(builder.context);
          }
        }
      }
      curr_instr = next_instr;
    }
    curr_bb = curr_bb->next;
  }
}

}  // namespace backend
}  // namespace syc