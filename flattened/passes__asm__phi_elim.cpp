#include "passes__asm__phi_elim.h"
#include "backend__basic_block.h"
#include "backend__instruction.h"
#include "backend__operand.h"
#include "ir__codegen.h"

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

    std::set<OperandID> phi_def_set;
    std::set<OperandID> conflict_set;

    // Detect if there might be a conflict
    while (curr_instr->is_phi() && curr_instr != curr_bb->tail_instruction) {
      auto phi = curr_instr->as<instruction::Phi>().value();

      for (auto [operand_id, _] : phi.incoming_list) {
        if (phi_def_set.count(operand_id)) {
          conflict_set.insert(operand_id);
        }
      }
      phi_def_set.insert(phi.rd_id);

      curr_instr = curr_instr->next;
    }

    std::map<BasicBlockID, BasicBlockID> block_jmp_map;

    std::map<BasicBlockID, std::vector<InstructionPtr>> block_instr_map;

    std::map<OperandID, OperandID> operand_map;

    curr_instr = curr_bb->head_instruction->next;
    while (curr_instr->is_phi() && curr_instr != curr_bb->tail_instruction) {
      auto next_instr = curr_instr->next;

      auto phi = std::get<instruction::Phi>(curr_instr->kind);

      auto phi_rd = builder.context.get_operand(phi.rd_id);

      OperandID phi_tmp_rd_id = phi_rd->id;

      if (conflict_set.count(phi_rd->id)) {
        if (phi_rd->is_float()) {
          phi_tmp_rd_id =
            builder.fetch_virtual_register(VirtualRegisterKind::Float);
        } else {
          phi_tmp_rd_id =
            builder.fetch_virtual_register(VirtualRegisterKind::General);
        }

        operand_map[phi.rd_id] = phi_tmp_rd_id;
      }

      for (const auto& [operand_id, pred_id] : phi.incoming_list) {
        auto operand = builder.context.get_operand(operand_id);
        auto pred_bb = builder.context.get_basic_block(pred_id);

        if (!pred_id_set.count(pred_id)) {
          continue;
        }

        auto block_to_insert = pred_id;

        if (pred_bb->succ_list.size() != 1) {
          if (block_jmp_map.count(pred_id)) {
            block_to_insert = block_jmp_map[pred_id];
          } else {
            auto new_bb = builder.fetch_basic_block();
            block_jmp_map[pred_id] = new_bb->id;
            block_to_insert = new_bb->id;

            builder.set_curr_basic_block(new_bb);

            curr_bb->remove_pred(pred_id);

            auto jmp_instr = builder.fetch_j_instruction(curr_bb->id);
            builder.append_instruction(jmp_instr);

            pred_bb->remove_succ(curr_bb->id);

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

            pred_bb->add_succ(new_bb->id);
            new_bb->add_pred(pred_bb->id);

            pred_bb->insert_next(new_bb);
          }
        }

        if (!block_instr_map.count(block_to_insert)) {
          block_instr_map[block_to_insert] = std::vector<InstructionPtr>();
        }

        if (operand->is_immediate()) {
          if (phi_rd->is_float()) {
            auto t5_id = builder.fetch_register(Register{GeneralRegister::T5});
            auto li_instr = builder.fetch_li_instruction(t5_id, operand_id);
            auto fmv_instr = builder.fetch_float_move_instruction(
              instruction::FloatMove::Fmt::S, instruction::FloatMove::Fmt::X,
              phi_tmp_rd_id, t5_id
            );
            block_instr_map[block_to_insert].push_back(li_instr);
            block_instr_map[block_to_insert].push_back(fmv_instr);
          } else {
            auto li_instr =
              builder.fetch_li_instruction(phi_tmp_rd_id, operand_id);
            block_instr_map[block_to_insert].push_back(li_instr);
          }
        } else if (operand->is_local_memory()) {
          auto offset = std::get<LocalMemory>(operand->kind).offset;
          auto asm_fp_id =
            builder.fetch_register(Register{GeneralRegister::S0});

          if (check_itype_immediate(offset)) {
            if (phi_rd->is_float()) {
              auto fld_instr = builder.fetch_float_load_instruction(
                instruction::FloatLoad::Op::FLD, phi_tmp_rd_id, asm_fp_id,
                builder.fetch_immediate(offset)
              );
              block_instr_map[block_to_insert].push_back(fld_instr);
            } else {
              auto ld_instr = builder.fetch_load_instruction(
                instruction::Load::Op::LD, phi_tmp_rd_id, asm_fp_id,
                builder.fetch_immediate(offset)
              );
              block_instr_map[block_to_insert].push_back(ld_instr);
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

            block_instr_map[block_to_insert].push_back(li_instruction);
            block_instr_map[block_to_insert].push_back(add_instruction);

            if (phi_rd->is_float()) {
              auto fld_instr = builder.fetch_float_load_instruction(
                instruction::FloatLoad::Op::FLD, phi_tmp_rd_id, asm_tmp_id,
                builder.fetch_immediate(0)
              );
              block_instr_map[block_to_insert].push_back(fld_instr);
            } else {
              auto ld_instr = builder.fetch_load_instruction(
                instruction::Load::Op::LD, phi_tmp_rd_id, asm_tmp_id,
                builder.fetch_immediate(0)
              );
              block_instr_map[block_to_insert].push_back(ld_instr);
            }
          }
        } else {
          if (phi_rd->is_float()) {
            auto fsgnjs_instr = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FSGNJ,
              backend::instruction::FloatBinary::S, phi_tmp_rd_id, operand_id,
              operand_id
            );
            block_instr_map[block_to_insert].push_back(fsgnjs_instr);
          } else {
            // mv
            auto addi_instr = builder.fetch_binary_imm_instruction(
              instruction::BinaryImm::ADDI, phi_tmp_rd_id, operand_id,
              builder.fetch_immediate(0)
            );
            block_instr_map[block_to_insert].push_back(addi_instr);
          }
        }
      }

      curr_instr->remove(builder.context);

      curr_instr = next_instr;
    }

    for (auto& [block_id, instr_list] : block_instr_map) {
      auto block = builder.context.get_basic_block(block_id);
      builder.set_curr_basic_block(block);

      auto exit_instr = block->tail_instruction->prev.lock();
      while (exit_instr != block->head_instruction &&
             exit_instr->is_branch_or_jmp()) {
        exit_instr = exit_instr->prev.lock();
      }

      exit_instr = exit_instr->next;

      for (auto& instr : instr_list) {
        exit_instr->insert_prev(instr);
        instr->parent_block_id = block_id;
      }
    }

    for (auto [rd_id, tmp_rd_id] : operand_map) {
      auto rd = builder.context.get_operand(rd_id);

      builder.set_curr_basic_block(curr_bb);

      if (rd->is_float()) {
        auto fsgnjs_instr = builder.fetch_float_binary_instruction(
          backend::instruction::FloatBinary::FSGNJ,
          backend::instruction::FloatBinary::S, rd_id, tmp_rd_id, tmp_rd_id
        );
        builder.prepend_instruction(fsgnjs_instr);
      } else {
        auto addi_instr = builder.fetch_binary_imm_instruction(
          instruction::BinaryImm::ADDI, rd_id, tmp_rd_id,
          builder.fetch_immediate(0)
        );
        builder.prepend_instruction(addi_instr);
      }
    }

    curr_bb = next_bb;
  }
}

}  // namespace backend
}  // namespace syc