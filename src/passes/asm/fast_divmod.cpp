#include "passes/asm/fast_divmod.h"

#include "backend/basic_block.h"
#include "backend/instruction.h"
#include "backend/operand.h"

namespace syc {
namespace backend {

// magic number for division
// from "Hacker's Delight", Henry S. Warren, Chapter 10.
struct ms {
  int M;  // Magic number
  int s;  // and shift amount.
};
struct ms magic(int d) {  // Must have 2 <= d <= 2**31-1 or -2**31 <= d <= -2.
  int p;
  unsigned ad, anc, delta, q1, r1, q2, r2, t;
  const unsigned two31 = 0x80000000;  // 2**31.
  struct ms mag;
  ad = abs(d);
  t = two31 + ((unsigned)d >> 31);
  anc = t - 1 - t % ad;   // Absolute value of nc.
  p = 31;                 // Init. p.
  q1 = two31 / anc;       // Init. q1 = 2**p/|nc|.
  r1 = two31 - q1 * anc;  // Init. r1 = rem(2**p, |nc|).
  q2 = two31 / ad;        // Init. q2 = 2**p/|d|.
  r2 = two31 - q2 * ad;   // Init. r2 = rem(2**p, |d|).
  do {
    p = p + 1;
    q1 = 2 * q1;      // Update q1 = 2**p/|nc|.
    r1 = 2 * r1;      // Update r1 = rem(2**p, |nc|).
    if (r1 >= anc) {  // (Must be an unsigned
      q1 = q1 + 1;    // comparison here.)
      r1 = r1 - anc;
    }
    q2 = 2 * q2;     // Update q2 = 2**p/|d|.
    r2 = 2 * r2;     // Update r2 = rem(2**p, |d|).
    if (r2 >= ad) {  // (Must be an unsigned
      q2 = q2 + 1;   // comparison here.)
      r2 = r2 - ad;
    }
    delta = ad - r2;
  } while (q1 < delta || (q1 == delta && r1 == 0));
  mag.M = q2 + 1;
  if (d < 0)
    mag.M = -mag.M;  // Magic number and
  mag.s = p - 32;    // shift amount to return.
  return mag;
}

void fast_divmod(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    fast_divmod_function(function, builder);
  }
}

void fast_divmod_function(FunctionPtr function, Builder& builder) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    fast_divmod_basic_block(curr_bb, builder);
    curr_bb = curr_bb->next;
  }
}

void fast_divmod_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;

  auto curr_instr = basic_block->head_instruction->next;
  while (curr_instr != basic_block->tail_instruction) {
    auto next_instr = curr_instr->next;

    if (curr_instr->is_li() && next_instr->is_binary()) {
      auto curr_li = curr_instr->as<Li>();
      auto next_binary = next_instr->as<Binary>();
      if (curr_li->rd_id == next_binary->rs2_id && next_binary->op == Binary::DIVW) {
        // li v0, c
        // divw v2, v1, v0
        auto immediate = std::get<Immediate>(
          builder.context.get_operand(curr_li->imm_id)->kind
        );
        if (immediate.get_value() > INT32_MAX || immediate.get_value() < INT32_MIN) {
          throw std::runtime_error("fast_divmod: immediate value out of range");
        }
        int immediate_value = (int)immediate.get_value();
        if (immediate_value == 0) {
          throw std::runtime_error("fast_divmod: immediate value cannot be zero");
        }
        auto next_dst = builder.context.get_operand(next_binary->rd_id);
        auto next_rs1 = builder.context.get_operand(next_binary->rs1_id);
        auto next_rs2 = builder.context.get_operand(next_binary->rs2_id);
        ms magic_number = magic(immediate_value);
        // std::cout << "magic number: " << magic_number.M << ", " << magic_number.s << std::endl;
        if (immediate_value >= 2) {
          if (magic_number.s > 0) {
            // if magic_number.s > 0
            // div v2, v1, c
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // addw v5, v4, v1
            // sraiw v6, v5, magic_number.s 
            // srliw v7, v1, 31
            // addw v2, v7, v6
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v6 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, next_dst->id, v7, v6)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAIW, v6, magic_number.M < 0 ? v5 : v4, builder.fetch_immediate(magic_number.s))
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          } else {
            // if magic_number.s <= 0
            // div v2, v1, c
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // addw v5, v4, v1
            // srliw v7, v1, 31
            // addw v2, v7, v5
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, next_dst->id, v7, magic_number.M < 0 ? v5 : v4)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          }
        } else if (immediate_value <= -2) {
          if (magic_number.s > 0) {
            // if magic_number.s > 0
            // div v2, v1, c
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // subw v5, v4, v1
            // sraiw v6, v5, magic_number.s 
            // srliw v7, v1, 31
            // addw v2, v7, v6
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v6 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, next_dst->id, v7, v6)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAIW, v6, magic_number.M < 0 ? v5 : v4, magic_number.s
              )
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          } else {
            // if magic_number.s <= 0
            // div v2, v1, c
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // subw v5, v4, v1
            // srliw v7, v1, 31
            // addw v2, v7, v5
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, next_dst->id, v7, magic_number.M < 0 ? v5 : v4)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          }
        } else {
          throw std::runtime_error("fast div not implemented");
        }

        curr_instr->remove(builder.context);
        curr_instr = next_instr;
        next_instr = next_instr->next;
        curr_instr->remove(builder.context);
      } else if (curr_li->rd_id == next_binary->rs2_id && next_binary->op == Binary::REMW) {
        // li v0, c
        // divw v2, v1, v0
        auto immediate = std::get<Immediate>(
          builder.context.get_operand(curr_li->imm_id)->kind
        );
        if (immediate.get_value() > INT32_MAX || immediate.get_value() < INT32_MIN) {
          throw std::runtime_error("fast_divmod: immediate value out of range");
        }
        int immediate_value = (int)immediate.get_value();
        if (immediate_value == 0) {
          throw std::runtime_error("fast_divmod: immediate value cannot be zero");
        }
        auto next_dst = builder.context.get_operand(next_binary->rd_id);
        auto next_rs1 = builder.context.get_operand(next_binary->rs1_id);
        auto next_rs2 = builder.context.get_operand(next_binary->rs2_id);
        ms magic_number = magic(immediate_value);
        // std::cout << "magic number: " << magic_number.M << ", " << magic_number.s << std::endl;
        if (immediate_value >= 2) {
          if (magic_number.s > 0) {
            // if magic_number.s > 0
            // li vc, c
            // rem v2, v1, vc
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // addw v5, v4, v1
            // sraiw v6, v5, magic_number.s 
            // srliw v7, v1, 31
            // addw v8, v7, v6
            // li v10, c
            // mulwi v9, v8, v10
            // subw v2, v1, v9
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v6 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v9 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v10 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, next_dst->id, next_rs1->id, v9)
            );
            next_instr->insert_next(  
              builder.fetch_binary_instruction(Binary::MULW,  v9, v8, v10)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v10, builder.fetch_immediate(immediate_value)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v8, v7, v6)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAIW, v6, magic_number.M < 0 ? v5 : v4, builder.fetch_immediate(magic_number.s))
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          } else {
            // if magic_number.s <= 0
            // div v2, v1, c
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // addw v5, v4, v1
            // srliw v7, v1, 31
            // addw v8, v7, v5
            // li v10, c
            // mulw v9, v8, v10
            // subw v2, v1, v9
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v9 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v10 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, next_dst->id, next_rs1->id, v9)
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MULW, v9, v8, v10)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v10, builder.fetch_immediate(immediate_value)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v8, v7, magic_number.M < 0 ? v5 : v4)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          }
        } else if (immediate_value <= -2) {
          if (magic_number.s > 0) {
            // if magic_number.s > 0
            // div v2, v1, c
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // subw v5, v4, v1
            // sraiw v6, v5, magic_number.s 
            // srliw v7, v1, 31
            // addw v8, v7, v6
            // li v10, c
            // mulw v9, v8, v10
            // subw v2, v1, v9
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v6 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v9 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v10 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, next_dst->id, next_rs1->id, v9)
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MULW, v9, v8, v10)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v10, builder.fetch_immediate(immediate_value)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v8, v7, v6)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAIW, v6, magic_number.M < 0 ? v5 : v4, magic_number.s
              )
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          } else {
            // if magic_number.s <= 0
            // div v2, v1, c
            // ->
            // li v8, magic_number.M
            // mul v3, v1, v8
            // srai v4, v3, 32
            // subw v5, v4, v1
            // srliw v7, v1, 31
            // addw v8, v7, v5
            // li v10, c
            // mulw v9, v8, v10
            // subw v2, v1, v9
            // in reverse order
            auto v3 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v4 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v5 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v7 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v8 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v9 = builder.fetch_virtual_register(VirtualRegisterKind::General);
            auto v10 = builder.fetch_virtual_register(VirtualRegisterKind::General);

            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, next_dst->id, next_rs1->id, v9)
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MULW, v9, v8, v10)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v10, builder.fetch_immediate(immediate_value)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::ADDW, v8, v7, magic_number.M < 0 ? v5 : v4)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRLIW, v7, next_rs1->id, builder.fetch_immediate(31))
            );
            if (magic_number.M < 0)
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::SUBW, v5, v4, next_rs1->id)
            );
            next_instr->insert_next(
              builder.fetch_binary_imm_instruction(
                BinaryImm::SRAI, v4, v3, builder.fetch_immediate(32)
              )
            );
            next_instr->insert_next(
              builder.fetch_binary_instruction(Binary::MUL, v3, next_rs1->id, v8)
            );
            next_instr->insert_next(
              builder.fetch_li_instruction(
                v8, builder.fetch_immediate(magic_number.M)
              )
            );
          }
        } else {
          throw std::runtime_error("fast div not implemented");
        }

        curr_instr->remove(builder.context);
        curr_instr = next_instr;
        next_instr = next_instr->next;
        curr_instr->remove(builder.context);
      }
    }
    curr_instr = next_instr;
  }
}

}  // namespace backend

}  // namespace syc