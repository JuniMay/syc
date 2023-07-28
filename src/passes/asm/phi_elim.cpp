#include "passes/asm/phi_elim.h"
#include "backend/basic_block.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "ir/codegen.h"

namespace syc {
namespace backend {

void phi_elim(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    builder.switch_function(function_name);
    phi_elim_function(function, builder);
  }
}

void phi_elim_function(FunctionPtr function, Builder& builder) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    auto next_bb = curr_bb->next;

    auto pred_id_set = std::set<BasicBlockID>();
    for (auto pred_id : curr_bb->pred_list) {
      pred_id_set.insert(pred_id);
    }

    auto curr_instr = curr_bb->head_instruction->next;

    std::map<BasicBlockID, BasicBlockID> block_jmp_map;

    while (curr_instr->is_phi() && curr_instr != curr_bb->tail_instruction) {
      auto next_instr = curr_instr->next;

      auto phi = std::get<instruction::Phi>(curr_instr->kind);

      auto phi_rd = builder.context.get_operand(phi.rd_id);

      for (const auto& [operand_id, pred_id] : phi.incoming_list) {
        auto operand = builder.context.get_operand(operand_id);
        auto pred_bb = builder.context.get_basic_block(pred_id);

        if (!pred_id_set.count(pred_id)) {
          continue;
        }

        if (pred_bb->succ_list.size() == 1) {
          auto exit_instr = pred_bb->tail_instruction->prev.lock();

          while (exit_instr->is_branch_or_jmp() &&
                 exit_instr != pred_bb->head_instruction) {
            exit_instr = exit_instr->prev.lock();
          }

          if (operand->is_immediate()) {
            if (phi_rd->is_float()) {
              auto t5_id =
                builder.fetch_register(Register{GeneralRegister::T5});
              auto li_instr = builder.fetch_li_instruction(t5_id, operand_id);
              exit_instr->insert_next(li_instr);
              auto fmv_instr = builder.fetch_float_move_instruction(
                instruction::FloatMove::Fmt::S, instruction::FloatMove::Fmt::X,
                phi.rd_id, t5_id
              );
              li_instr->insert_next(fmv_instr);
            } else {
              auto li_instr =
                builder.fetch_li_instruction(phi.rd_id, operand_id);
              exit_instr->insert_next(li_instr);
            }
          } else if (operand->is_local_memory()) {
            // Local memory in phi is parameter
            auto offset = std::get<LocalMemory>(operand->kind).offset;
            auto asm_fp_id = builder.fetch_register(backend::Register{
              backend::GeneralRegister::S0});
            if (check_itype_immediate(offset)) {
              auto ld_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LD, phi_rd->id, asm_fp_id,
                builder.fetch_immediate(offset)
              );
              exit_instr->insert_next(ld_instruction);
            } else {
              auto asm_tmp_id = builder.fetch_register(backend::Register{
                backend::GeneralRegister::T3});
              auto li_instruction = builder.fetch_li_instruction(
                asm_tmp_id, builder.fetch_immediate(offset)
              );
              exit_instr->insert_next(li_instruction);
              auto add_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::ADD, asm_tmp_id, asm_fp_id,
                asm_tmp_id
              );
              li_instruction->insert_next(add_instruction);
              auto ld_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LD, phi_rd->id, asm_tmp_id,
                builder.fetch_immediate(0)
              );
              add_instruction->insert_next(ld_instruction);
            }
          } else {
            if (phi_rd->is_float()) {
              auto fsgnjs_instr = builder.fetch_float_binary_instruction(
                backend::instruction::FloatBinary::FSGNJ,
                backend::instruction::FloatBinary::S, phi.rd_id, operand_id,
                operand_id
              );
              exit_instr->insert_next(fsgnjs_instr);
            } else {
              // mv
              auto addi_instr = builder.fetch_binary_imm_instruction(
                instruction::BinaryImm::ADDI, phi.rd_id, operand_id,
                builder.fetch_immediate(0)
              );
              exit_instr->insert_next(addi_instr);
            }
          }
        } else {
          // add a new block
          BasicBlockPtr new_bb;
          bool replace_pred_jmp = true;
          if (block_jmp_map.count(pred_id)) {
            new_bb = builder.context.get_basic_block(block_jmp_map[pred_id]);
            replace_pred_jmp = false;
          } else {
            new_bb = builder.fetch_basic_block();
            block_jmp_map[pred_id] = new_bb->id;
          }

          builder.set_curr_basic_block(new_bb);

          if (operand->is_immediate()) {
            if (phi_rd->is_float()) {
              auto t5_id =
                builder.fetch_register(Register{GeneralRegister::T5});
              auto li_instr = builder.fetch_li_instruction(t5_id, operand_id);
              auto fmv_instr = builder.fetch_float_move_instruction(
                instruction::FloatMove::Fmt::S, instruction::FloatMove::Fmt::X,
                phi.rd_id, t5_id
              );
              if (replace_pred_jmp) {
                builder.append_instruction(li_instr);
                builder.append_instruction(fmv_instr);
              } else {
                auto exit_instr = new_bb->tail_instruction->prev.lock();
                while (exit_instr->is_branch_or_jmp() &&
                       exit_instr != new_bb->head_instruction) {
                  exit_instr = exit_instr->prev.lock();
                }
                exit_instr->insert_next(li_instr);
                li_instr->insert_next(fmv_instr);
              }

            } else {
              auto li_instr =
                builder.fetch_li_instruction(phi.rd_id, operand_id);
              if (replace_pred_jmp) {
                builder.append_instruction(li_instr);
              } else {
                auto exit_instr = new_bb->tail_instruction->prev.lock();
                while (exit_instr->is_branch_or_jmp() &&
                       exit_instr != new_bb->head_instruction) {
                  exit_instr = exit_instr->prev.lock();
                }
                exit_instr->insert_next(li_instr);
              }
            }
          } else if (operand->is_local_memory()) {
            // Local memory in phi is parameter
            auto offset = std::get<LocalMemory>(operand->kind).offset;
            auto asm_fp_id = builder.fetch_register(backend::Register{
              backend::GeneralRegister::S0});
            if (check_itype_immediate(offset)) {
              auto ld_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LD, phi_rd->id, asm_fp_id,
                builder.fetch_immediate(offset)
              );
              if (replace_pred_jmp) {
                builder.append_instruction(ld_instruction);
              } else {
                auto exit_instr = new_bb->tail_instruction->prev.lock();
                while (exit_instr->is_branch_or_jmp() &&
                       exit_instr != new_bb->head_instruction) {
                  exit_instr = exit_instr->prev.lock();
                }
                exit_instr->insert_next(ld_instruction);
              }
            } else {
              auto asm_tmp_id = builder.fetch_register(backend::Register{
                backend::GeneralRegister::T3});
              auto li_instruction = builder.fetch_li_instruction(
                asm_tmp_id, builder.fetch_immediate(offset)
              );
              auto add_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::ADD, asm_tmp_id, asm_fp_id,
                asm_tmp_id
              );
              auto ld_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LD, phi_rd->id, asm_tmp_id,
                builder.fetch_immediate(0)
              );
              if (replace_pred_jmp) {
                builder.append_instruction(li_instruction);
                builder.append_instruction(add_instruction);
                builder.append_instruction(ld_instruction);
              } else {
                auto exit_instr = new_bb->tail_instruction->prev.lock();
                while (exit_instr->is_branch_or_jmp() &&
                       exit_instr != new_bb->head_instruction) {
                  exit_instr = exit_instr->prev.lock();
                }
                exit_instr->insert_next(li_instruction);
                li_instruction->insert_next(add_instruction);
                add_instruction->insert_next(ld_instruction);
              }
            }
          } else {
            InstructionPtr new_instr;

            if (phi_rd->is_float()) {
              new_instr = builder.fetch_float_binary_instruction(
                backend::instruction::FloatBinary::FSGNJ,
                backend::instruction::FloatBinary::S, phi.rd_id, operand_id,
                operand_id
              );
            } else {
              // mv
              new_instr = builder.fetch_binary_imm_instruction(
                instruction::BinaryImm::ADDI, phi.rd_id, operand_id,
                builder.fetch_immediate(0)
              );
            }

            if (replace_pred_jmp) {
              builder.append_instruction(new_instr);
            } else {
              auto exit_instr = new_bb->tail_instruction->prev.lock();
              while (exit_instr->is_branch_or_jmp() &&
                     exit_instr != new_bb->head_instruction) {
                exit_instr = exit_instr->prev.lock();
              }
              exit_instr->insert_next(new_instr);
            }
          }

          if (replace_pred_jmp) {
            auto jmp_instr = builder.fetch_j_instruction(curr_bb->id);
            builder.append_instruction(jmp_instr);
            pred_bb->insert_next(new_bb);
          }

          builder.set_curr_basic_block(pred_bb);

          if (replace_pred_jmp) {
            // modify jump or branch
            auto exit_instr = pred_bb->tail_instruction->prev.lock();
            while (exit_instr->is_branch_or_jmp() &&
                   exit_instr != pred_bb->head_instruction) {
              if (auto jmp = std::get_if<instruction::J>(&exit_instr->kind)) {
                if (jmp->block_id == curr_bb->id) {
                  jmp->block_id = new_bb->id;
                  break;
                }
              } else if (auto branch =
                         std::get_if<instruction::Branch>(&exit_instr->kind)) {
                if (branch->block_id == curr_bb->id) {
                  branch->block_id = new_bb->id;
                  break;
                }
              }
              exit_instr = exit_instr->prev.lock();
            }
            pred_bb->remove_succ(curr_bb->id);
            curr_bb->remove_pred(pred_bb->id);

            pred_bb->add_succ(new_bb->id);
            new_bb->add_pred(pred_bb->id);
          }
        }
      }

      curr_instr->remove(builder.context);

      curr_instr = next_instr;
    }

    curr_bb = next_bb;
  }
}

}  // namespace backend
}  // namespace syc