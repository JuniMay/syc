#include "passes/asm/addr_simplification.h"
#include "backend/basic_block.h"
#include "backend/builder.h"
#include "backend/context.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "passes/asm/control_flow_analysis.h"

namespace syc {
namespace backend {

void addr_simplification(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    addr_simplification_function(function, builder);
  }
}

void addr_simplification_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    addr_simplification_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void addr_simplification_basic_block(
  BasicBlockPtr basic_block,
  Builder& builder
) {
  using namespace instruction;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    if (curr_instruction->is_binary_imm()) {
      auto curr_binary = curr_instruction->as<BinaryImm>();
      if (curr_binary->op == BinaryImm::Op::ADDI || curr_binary->op == BinaryImm::Op::ADDIW) {
        auto rd_id = curr_binary->rd_id;
        auto rs_id = curr_binary->rs_id;
        auto imm_id = curr_binary->imm_id;
        // find addi vn, sp, imm
        auto rd_operand = builder.context.get_operand(rd_id);
        auto rs_operand = builder.context.get_operand(rs_id);
        auto imm_operand = builder.context.get_operand(imm_id);
        if (rd_operand->def_id_list.size() == 1 && rs_operand->is_sp()) {
          auto imm = std::get<Immediate>(imm_operand->kind);
          auto imm_value = imm.value;
          // std::cout << "find addi " << rd_id << ", sp, " << "imm" <<
          // std::endl;
          bool do_remove = true;
          auto use_id_list_copy = rd_operand->use_id_list;
          for (InstructionID use : use_id_list_copy) {
            // std::cout << "use: " << use << std::endl;
            auto use_inst = builder.context.get_instruction(use);
            if (use_inst->is_load()) {
              // std::cout << "replace load" << std::endl;
              auto use_ld_inst = std::get<Load>(use_inst->kind);
              auto use_ld_rd = use_ld_inst.rd_id;
              auto use_ld_rs = use_ld_inst.rs_id;
              auto use_ld_imm = use_ld_inst.imm_id;

              auto use_ld_rd_operand = builder.context.get_operand(use_ld_rd);
              auto use_ld_rs_operand = builder.context.get_operand(use_ld_rs);
              auto use_ld_imm_operand = builder.context.get_operand(use_ld_imm);

              auto old_imm = std::get<Immediate>(use_ld_imm_operand->kind);
              auto old_imm_value = old_imm.value;

              auto new_imm_value = add_immediate(old_imm_value, imm_value);
              auto new_imm = builder.fetch_immediate(new_imm_value);
              auto new_rs_operand = builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp});

              use_inst->replace_operand(
                use_ld_rs, new_rs_operand, builder.context
              );
              use_inst->replace_operand(use_ld_imm, new_imm, builder.context);
            } else if (use_inst->is_store()) {
              // std::cout << "replace store" << std::endl;
              auto use_st_inst = std::get<Store>(use_inst->kind);
              auto use_st_rs1_id = use_st_inst.rs1_id;
              auto use_st_imm_id = use_st_inst.imm_id;

              if (use_st_rs1_id != rd_operand->id) {
                do_remove = false;
                continue;
              }

              auto use_st_rs1_operand =
                builder.context.get_operand(use_st_rs1_id);
              auto use_st_imm_operand =
                builder.context.get_operand(use_st_imm_id);

              auto old_imm = std::get<Immediate>(use_st_imm_operand->kind);
              auto old_imm_value = old_imm.value;

              auto new_imm_value = add_immediate(old_imm_value, imm_value);
              auto new_imm = builder.fetch_immediate(new_imm_value);
              auto new_rs_operand = builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp});

              use_inst->replace_operand(
                use_st_rs1_id, new_rs_operand, builder.context
              );
              use_inst->replace_operand(
                use_st_imm_id, new_imm, builder.context
              );
            } else {
              do_remove = false;
            }
          }

          if (do_remove) {
            curr_instruction->remove(builder.context);
            // std::cout << "remove" << std::endl;
          }
        }
      }
    }

    curr_instruction = next_instruction;
  }
}

std::variant<int32_t, int64_t, uint32_t, uint64_t> add_immediate(
  std::variant<int32_t, int64_t, uint32_t, uint64_t> a,
  std::variant<int32_t, int64_t, uint32_t, uint64_t> b
) {
  if (std::holds_alternative<int32_t>(a)) {
    if (std::holds_alternative<int32_t>(b)) {
      return std::get<int32_t>(a) + std::get<int32_t>(b);
    } else if (std::holds_alternative<int64_t>(b)) {
      return std::get<int32_t>(a) + std::get<int64_t>(b);
    } else if (std::holds_alternative<uint32_t>(b)) {
      return std::get<int32_t>(a) + std::get<uint32_t>(b);
    } else {
      return std::get<int32_t>(a) + std::get<uint64_t>(b);
    }
  } else if (std::holds_alternative<int64_t>(a)) {
    if (std::holds_alternative<int32_t>(b)) {
      return std::get<int64_t>(a) + std::get<int32_t>(b);
    } else if (std::holds_alternative<int64_t>(b)) {
      return std::get<int64_t>(a) + std::get<int64_t>(b);
    } else if (std::holds_alternative<uint32_t>(b)) {
      return std::get<int64_t>(a) + std::get<uint32_t>(b);
    } else {
      return std::get<int64_t>(a) + std::get<uint64_t>(b);
    }
  } else if (std::holds_alternative<uint32_t>(a)) {
    if (std::holds_alternative<int32_t>(b)) {
      return std::get<uint32_t>(a) + std::get<int32_t>(b);
    } else if (std::holds_alternative<int64_t>(b)) {
      return std::get<uint32_t>(a) + std::get<int64_t>(b);
    } else if (std::holds_alternative<uint32_t>(b)) {
      return std::get<uint32_t>(a) + std::get<uint32_t>(b);
    } else {
      return std::get<uint32_t>(a) + std::get<uint64_t>(b);
    }
  } else {
    if (std::holds_alternative<int32_t>(b)) {
      return std::get<uint64_t>(a) + std::get<int32_t>(b);
    } else if (std::holds_alternative<int64_t>(b)) {
      return std::get<uint64_t>(a) + std::get<int64_t>(b);
    } else if (std::holds_alternative<uint32_t>(b)) {
      return std::get<uint64_t>(a) + std::get<uint32_t>(b);
    } else {
      return std::get<uint64_t>(a) + std::get<uint64_t>(b);
    }
  }
}

}  // namespace backend
}  // namespace syc