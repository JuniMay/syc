#include "passes__asm__peephole_final.h"

#include "backend__basic_block.h"
#include "backend__instruction.h"
#include "backend__operand.h"

namespace syc {
namespace backend {

void peephole_final(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    peephole_final_function(function, builder);
  }
}

void peephole_final_function(FunctionPtr function, Builder& builder) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    peephole_final_basic_block(curr_bb, builder);
    curr_bb = curr_bb->next;
  }
}

void peephole_final_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;

  auto curr_instr = basic_block->head_instruction->next;
  while (curr_instr != basic_block->tail_instruction) {
    auto next_instr = curr_instr->next;

    auto maybe_binary_imm = curr_instr->as<BinaryImm>();

    if (maybe_binary_imm.has_value()) {
      if (maybe_binary_imm->op == BinaryImm::ADDI || maybe_binary_imm->op == BinaryImm::ADDIW) {
        // Remove: addi r, r, 0
        auto binary_imm = maybe_binary_imm.value();
        auto dst = builder.context.get_operand(binary_imm.rd_id);
        auto lhs = builder.context.get_operand(binary_imm.rs_id);
        auto imm = builder.context.get_operand(binary_imm.imm_id);

        auto rd = std::get<Register>(dst->kind);
        auto rs = std::get<Register>(lhs->kind);

        if (rd == rs && imm->is_zero()) {
          curr_instr->remove(builder.context);
        }
      }
    }

    curr_instr = curr_instr->next;
  }
}

}  // namespace backend

}  // namespace syc