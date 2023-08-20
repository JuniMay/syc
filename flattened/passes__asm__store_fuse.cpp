#include "passes__asm__store_fuse.h"
#include "backend__basic_block.h"
#include "backend__builder.h"
#include "backend__context.h"
#include "backend__function.h"
#include "backend__instruction.h"
#include "backend__operand.h"

namespace syc {
namespace backend {

void store_fuse(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    store_fuse_function(function, builder);
  }
}

void store_fuse_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    store_fuse_simple_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }

  // curr_basic_block = function->head_basic_block->next;
  // while (curr_basic_block != function->tail_basic_block) {
  //   store_fuse_compl_basic_block(curr_basic_block, builder);
  //   curr_basic_block = curr_basic_block->next;
  // }
}

void store_fuse_simple_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;

  auto first_instruction = basic_block->head_instruction->next;
  while (first_instruction != basic_block->tail_instruction) {
    auto second_instruction = first_instruction->next;
    if (second_instruction == basic_block->tail_instruction) {
      break;
    }

    if (first_instruction->is_store() && second_instruction->is_store()) {
      auto first_store = first_instruction->as<Store>();
      auto second_store = second_instruction->as<Store>();
      auto first_op = first_store->op;
      auto second_op = second_store->op;

      // sw zero, c(v1)
      // sw zero, c+4(v1)
      // ->
      // sd zero, c(v1)

      if (first_op == Store::Op::SW && second_op == Store::Op::SW) {
        auto first_rs1_id = first_store->rs1_id;
        auto second_rs1_id = second_store->rs1_id;
        auto first_rs2_id = first_store->rs2_id;
        auto second_rs2_id = second_store->rs2_id;
        auto first_imm_id = first_store->imm_id;
        auto second_imm_id = second_store->imm_id;
        auto first_rs1 = builder.context.get_operand(first_rs1_id);
        auto second_rs1 = builder.context.get_operand(second_rs1_id);
        auto first_rs2 = builder.context.get_operand(first_rs2_id);
        auto second_rs2 = builder.context.get_operand(second_rs2_id);
        auto first_imm = builder.context.get_operand(first_store->imm_id);
        auto first_imm_val = 
          std::get<int32_t>(std::get<Immediate>(first_imm->kind).value);
        auto second_imm = builder.context.get_operand(second_store->imm_id);
        auto second_imm_val = 
          std::get<int32_t>(std::get<Immediate>(second_imm->kind).value);
        if (
          first_rs1_id == second_rs1_id &&
          first_rs2->is_zero() &&
          second_rs2->is_zero() &&
          first_imm_val + 4 == second_imm_val
        ) {
          auto new_store = builder.fetch_store_instruction(
            Store::Op::SD,
            first_rs1_id,
            first_rs2_id,
            first_imm_id
          );
          second_instruction->insert_next(new_store);
          first_instruction->remove(builder.context);
          second_instruction->remove(builder.context);
          second_instruction = new_store;
        }
      }
    }

    first_instruction = second_instruction;
  }
}

void store_fuse_compl_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;

  auto first_instruction = basic_block->head_instruction->next;
  while (first_instruction != basic_block->tail_instruction) {
    auto second_instruction = first_instruction->next;
    if (second_instruction == basic_block->tail_instruction) {
      break;
    }
    auto third_instruction = second_instruction->next;
    if (third_instruction == basic_block->tail_instruction) {
      break;
    }
    auto fourth_instruction = third_instruction->next;
    if (fourth_instruction == basic_block->tail_instruction) {
      break;
    }
    if (first_instruction->is_li() &&
        second_instruction->is_store() &&
        third_instruction->is_li() &&
        fourth_instruction->is_store()) {
      auto first_li = first_instruction->as<Li>();
      auto second_store = second_instruction->as<Store>();
      auto third_li = third_instruction->as<Li>();
      auto fourth_store = fourth_instruction->as<Store>();
      auto first_rd_id = first_li->rd_id;
      auto first_imm_id = first_li->imm_id;
      auto second_rs1_id = second_store->rs1_id;
      auto second_rs2_id = second_store->rs2_id;
      auto second_imm_id = second_store->imm_id;
      auto third_rd_id = third_li->rd_id;
      auto third_imm_id = third_li->imm_id;
      auto fourth_rs1_id = fourth_store->rs1_id;
      auto fourth_rs2_id = fourth_store->rs2_id;
      auto fourth_imm_id = fourth_store->imm_id;
      auto second_imm = builder.context.get_operand(second_imm_id);
      auto second_imm_val = 
        std::get<int32_t>(std::get<Immediate>(second_imm->kind).value);
      auto fourth_imm = builder.context.get_operand(fourth_imm_id);
      auto fourth_imm_val = 
        std::get<int32_t>(std::get<Immediate>(fourth_imm->kind).value);
      if (
        first_rd_id == second_rs2_id &&
        third_rd_id == fourth_rs2_id &&
        second_rs1_id == fourth_rs1_id &&
        second_imm_val + 4 == fourth_imm_val
      ) {
        auto first_imm = builder.context.get_operand(first_imm_id);
        auto first_imm_val = 
          std::get<int32_t>(std::get<Immediate>(first_imm->kind).value);
        auto third_imm = builder.context.get_operand(third_imm_id);
        auto third_imm_val = 
          std::get<int32_t>(std::get<Immediate>(third_imm->kind).value);
        auto new_imm_val = (int64_t)third_imm_val << 32 | (int64_t)first_imm_val;
        auto new_imm_id = builder.fetch_immediate(new_imm_val);
        auto new_li = builder.fetch_li_instruction(
          first_rd_id,
          new_imm_id
        );
        auto new_store = builder.fetch_store_instruction(
          Store::Op::SD,
          second_rs1_id,
          second_rs2_id,
          second_imm_id
        );
        fourth_instruction->insert_next(new_li);
        new_li->insert_next(new_store);
        first_instruction->remove(builder.context);
        second_instruction->remove(builder.context);
        third_instruction->remove(builder.context);
        fourth_instruction->remove(builder.context);
        second_instruction = new_store;
      }
    } else if (first_instruction->is_lui() &&
        second_instruction->is_store() &&
        third_instruction->is_lui() &&
        fourth_instruction->is_store()) {
      auto first_li = first_instruction->as<Lui>();
      auto second_store = second_instruction->as<Store>();
      auto third_li = third_instruction->as<Lui>();
      auto fourth_store = fourth_instruction->as<Store>();
      auto first_rd_id = first_li->rd_id;
      auto first_imm_id = first_li->imm_id;
      auto second_rs1_id = second_store->rs1_id;
      auto second_rs2_id = second_store->rs2_id;
      auto second_imm_id = second_store->imm_id;
      auto third_rd_id = third_li->rd_id;
      auto third_imm_id = third_li->imm_id;
      auto fourth_rs1_id = fourth_store->rs1_id;
      auto fourth_rs2_id = fourth_store->rs2_id;
      auto fourth_imm_id = fourth_store->imm_id;
      auto second_imm = builder.context.get_operand(second_imm_id);
      auto second_imm_val = 
        std::get<int32_t>(std::get<Immediate>(second_imm->kind).value);
      auto fourth_imm = builder.context.get_operand(fourth_imm_id);
      auto fourth_imm_val = 
        std::get<int32_t>(std::get<Immediate>(fourth_imm->kind).value);
      if (
        first_rd_id == second_rs2_id &&
        third_rd_id == fourth_rs2_id &&
        second_rs1_id == fourth_rs1_id &&
        second_imm_val + 4 == fourth_imm_val
      ) {
        auto first_imm = builder.context.get_operand(first_imm_id);
        auto first_imm_val = 
          std::get<uint32_t>(std::get<Immediate>(first_imm->kind).value);
        auto third_imm = builder.context.get_operand(third_imm_id);
        auto third_imm_val = 
          std::get<uint32_t>(std::get<Immediate>(third_imm->kind).value);
        auto new_imm_val = (int64_t)third_imm_val << 44 | (int64_t)first_imm_val << 12;
        auto new_imm_id = builder.fetch_immediate(new_imm_val);
        auto new_li = builder.fetch_li_instruction(
          first_rd_id,
          new_imm_id
        );
        auto new_store = builder.fetch_store_instruction(
          Store::Op::SD,
          second_rs1_id,
          second_rs2_id,
          second_imm_id
        );
        fourth_instruction->insert_next(new_li);
        new_li->insert_next(new_store);
        first_instruction->remove(builder.context);
        second_instruction->remove(builder.context);
        third_instruction->remove(builder.context);
        fourth_instruction->remove(builder.context);
        second_instruction = new_store;
      }
    }

    first_instruction = second_instruction;
  }    
}

}  // namespace backend
}  // namespace syc