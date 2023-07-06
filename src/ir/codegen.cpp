#include "ir/codegen.h"

namespace syc {

AsmOperandID CodegenContext::get_asm_operand_id(IrOperandID ir_operand_id) {
  try {
    return operand_map.at(ir_operand_id);
  } catch (std::out_of_range& e) {
    throw std::runtime_error(
      "operand not found " + std::to_string(ir_operand_id)
    );
  }
}

void codegen(
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  for (auto& ir_operand_id : ir_context.global_list) {
    auto& ir_global = std::get<ir::operand::Global>(
      ir_context.operand_table[ir_operand_id]->kind
    );
    auto ir_init_constant = std::get<ir::operand::ConstantPtr>(
      ir_context.operand_table[ir_global.init]->kind
    );

    auto& ir_init_constant_kind = ir_init_constant->kind;
    auto ir_init_constant_type = ir_init_constant->type;

    if (std::holds_alternative<ir::type::Integer>(*ir_init_constant_type)) {
      int value = std::get<int>(ir_init_constant_kind);
      // bitwise conversion
      std::vector<uint32_t> asm_value = {*reinterpret_cast<uint32_t*>(&value)};
      auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
      codegen_context.operand_map[ir_operand_id] = asm_operand_id;

    } else if (std::holds_alternative<ir::type::Float>(*ir_init_constant_type
               )) {
      float value = std::get<float>(ir_init_constant_kind);
      // bitwise conversion
      std::vector<uint32_t> asm_value = {*reinterpret_cast<uint32_t*>(&value)};
      auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
      codegen_context.operand_map[ir_operand_id] = asm_operand_id;

    } else if (std::holds_alternative<ir::type::Array>(*ir_init_constant_type
               )) {
      if (std::holds_alternative<ir::operand::Zeroinitializer>(
            ir_init_constant_kind
          )) {
        // zeroinitializer directly convert to zero-initialized symbol in bss.
        auto asm_operand_id = builder.fetch_global(
          ir_global.name, ir::get_size(ir_init_constant_type) / 8
        );
        codegen_context.operand_map[ir_operand_id] = asm_operand_id;

      } else {
        // flatten multidimensional array
        std::vector<uint32_t> asm_value;

        std::function<void(ir::operand::ConstantPtr&)> recursive_func =
          [&](ir::operand::ConstantPtr& ir_constant) {
            if (std::holds_alternative<ir::type::Array>(*ir_constant->type)) {
              auto& ir_constant_kind = ir_constant->kind;
              if (auto ir_constant_list = std::get_if<std::vector<ir::operand::ConstantPtr> >(&ir_constant_kind)) {
                for (auto& ir_constant_element : *ir_constant_list) {
                  recursive_func(ir_constant_element);
                }
              } else {
                // Zeroinitializer
                auto zero_cnt = ir::get_size(ir_constant->type) / 32;
                for (size_t i = 0; i < zero_cnt; ++i) {
                  asm_value.push_back(0);
                }
              }
            } else {
              auto ir_constant_type = ir_constant->type;
              if (std::holds_alternative<ir::type::Integer>(*ir_constant_type
                  )) {
                int value = std::get<int>(ir_constant->kind);
                // bitwise conversion
                uint32_t asm_value_element =
                  *reinterpret_cast<uint32_t*>(&value);
                asm_value.push_back(asm_value_element);

              } else if (std::holds_alternative<ir::type::Float>(
                           *ir_constant_type
                         )) {
                float value = std::get<float>(ir_constant->kind);
                // bitwise conversion
                uint32_t asm_value_element =
                  *reinterpret_cast<uint32_t*>(&value);
                asm_value.push_back(asm_value_element);

              } else {
                throw std::runtime_error(
                  "Invalid type for global variable initialization."
                );
              }
            }
          };

        recursive_func(ir_init_constant);

        auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
        codegen_context.operand_map[ir_operand_id] = asm_operand_id;
      }
    } else {
      throw std::runtime_error(
        "Invalid type for global variable initialization."
      );
    }
  }

  for (auto& [ir_function_name, ir_function] : ir_context.function_table) {
    if (ir_function->is_declare) {
      continue;
    }
    codegen_function(ir_function, ir_context, builder, codegen_context);
  }

  asm_register_allocation(builder);

  for (auto& [ir_function_name, ir_function] : ir_context.function_table) {
    if (ir_function->is_declare) {
      continue;
    }
    // adjust stack frame size to be 16-byte aligned
    codegen_function_prolouge(ir_function_name, builder, codegen_context);
    codegen_function_epilouge(ir_function_name, builder, codegen_context);
  }
}

void codegen_function(
  IrFunctionPtr ir_function,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  builder.add_function(ir_function->name);
  builder.switch_function(ir_function->name);

  auto curr_ir_basic_block = ir_function->head_basic_block->next;
  while (curr_ir_basic_block != ir_function->tail_basic_block) {
    codegen_basic_block(
      curr_ir_basic_block, ir_context, builder, codegen_context
    );
    curr_ir_basic_block = curr_ir_basic_block->next;
  }
}

void codegen_function_prolouge(
  std::string function_name,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  using namespace backend;

  auto asm_function = builder.context.get_function(function_name);
  auto entry_block = asm_function->head_basic_block->next;

  auto stack_frame_size = asm_function->stack_frame_size;

  // store ra
  auto ra_id = builder.fetch_register(Register{GeneralRegister::Ra});

  auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

  stack_frame_size +=
    8 * (1 + asm_function->saved_general_register_list.size() +
         asm_function->saved_float_register_list.size());

  asm_function->stack_frame_size = stack_frame_size;

  // Adjust frame size to be 16-byte aligned
  // There might be gaps between saved registers and local variables.
  size_t aligned_stack_frame_size = (stack_frame_size + 15) / 16 * 16;

  asm_function->align_frame_size =
    aligned_stack_frame_size - asm_function->stack_frame_size;

  size_t curr_frame_pos = aligned_stack_frame_size - 16;

  for (auto reg : asm_function->saved_general_register_list) {
    auto reg_id = builder.fetch_register(map_general_register(reg));
    auto sd_instruction = builder.fetch_store_instruction(
      instruction::Store::Op::SD, sp_id, reg_id,
      builder.fetch_immediate((int32_t)curr_frame_pos)
    );
    entry_block->prepend_instruction(sd_instruction);
    curr_frame_pos -= 8;
  }

  for (auto reg : asm_function->saved_float_register_list) {
    auto reg_id = builder.fetch_register(map_float_register(reg));
    auto fsd_instruction = builder.fetch_float_store_instruction(
      instruction::FloatStore::Op::FSD, sp_id, reg_id,
      builder.fetch_immediate((int32_t)curr_frame_pos)
    );
    entry_block->prepend_instruction(fsd_instruction);
    curr_frame_pos -= 8;
  }

  auto sd_instruction = builder.fetch_store_instruction(
    instruction::Store::Op::SD, sp_id, ra_id,
    builder.fetch_immediate((int32_t)(aligned_stack_frame_size - 8))
  );

  entry_block->prepend_instruction(sd_instruction);

  auto addi_instruction = builder.fetch_binary_imm_instruction(
    instruction::BinaryImm::Op::ADDI, sp_id, sp_id,
    builder.fetch_immediate(-(int32_t)aligned_stack_frame_size)
  );
  entry_block->prepend_instruction(addi_instruction);
}

void codegen_function_epilouge(
  std::string function_name,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  using namespace backend;

  auto asm_function = builder.context.get_function(function_name);
  auto exit_block = asm_function->tail_basic_block->prev.lock();

  auto stack_frame_size = asm_function->stack_frame_size;
  auto align_frame_size = asm_function->align_frame_size;

  // ret
  auto last_instruction = exit_block->tail_instruction->prev.lock();

  // restore ra
  auto ra_id = builder.fetch_register(Register{GeneralRegister::Ra});

  auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

  auto ld_instruction = builder.fetch_load_instruction(
    instruction::Load::Op::LD, ra_id, sp_id,
    builder.fetch_immediate((int32_t)(stack_frame_size + align_frame_size - 8))
  );

  auto addi_instruction = builder.fetch_binary_imm_instruction(
    instruction::BinaryImm::Op::ADDI, sp_id, sp_id,
    builder.fetch_immediate((int32_t)(stack_frame_size + align_frame_size))
  );

  size_t aligned_stack_frame_size =
    asm_function->stack_frame_size + asm_function->align_frame_size;
  size_t curr_frame_pos = aligned_stack_frame_size - 16;

  for (auto reg : asm_function->saved_general_register_list) {
    auto reg_id = builder.fetch_register(map_general_register(reg));
    auto ld_instruction = builder.fetch_load_instruction(
      instruction::Load::Op::LD, reg_id, sp_id,
      builder.fetch_immediate((int32_t)curr_frame_pos)
    );
    last_instruction->insert_prev(ld_instruction);
    curr_frame_pos -= 8;
  }

  for (auto reg : asm_function->saved_float_register_list) {
    auto reg_id = builder.fetch_register(map_float_register(reg));
    auto fsd_instruction = builder.fetch_float_load_instruction(
      instruction::FloatLoad::Op::FLD, reg_id, sp_id,
      builder.fetch_immediate((int32_t)curr_frame_pos)
    );
    last_instruction->insert_prev(fsd_instruction);
    curr_frame_pos -= 8;
  }

  last_instruction->insert_prev(ld_instruction);
  last_instruction->insert_prev(addi_instruction);
}

void codegen_basic_block(
  IrBasicBlockPtr ir_basic_block,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  if (codegen_context.basic_block_map.count(ir_basic_block->id)) {
    return;
  }

  auto basic_block = builder.fetch_basic_block();

  codegen_context.basic_block_map[ir_basic_block->id] = basic_block->id;

  builder.curr_function->append_basic_block(basic_block);

  builder.set_curr_basic_block(basic_block);

  auto curr_ir_instruction = ir_basic_block->head_instruction->next;
  while (curr_ir_instruction != ir_basic_block->tail_instruction) {
    codegen_instruction(
      curr_ir_instruction, ir_context, builder, codegen_context
    );
    curr_ir_instruction = curr_ir_instruction->next;
  }

}

void codegen_instruction(
  IrInstructionPtr ir_instruction,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  std::visit(
    overloaded{
      [&](ir::instruction::Alloca& ir_alloca) {
        auto ir_allocated_type = ir_alloca.allocated_type;
        auto ir_allocated_size = ir::get_size(ir_allocated_type);

        auto curr_frame_size = builder.curr_function->stack_frame_size;
        auto asm_allocated_size = ir_allocated_size / 8;
        auto asm_local_memory_id = builder.fetch_local_memory(curr_frame_size);
        builder.curr_function->stack_frame_size += asm_allocated_size;

        codegen_context.operand_map[ir_alloca.dst_id] = asm_local_memory_id;
      },
      [&](ir::instruction::Store& ir_store) {
        auto ir_value_id = ir_store.value_id;
        auto ir_ptr_id = ir_store.ptr_id;

        auto asm_value_id = codegen_operand(
          ir_value_id, ir_context, builder, codegen_context, false, false
        );
        auto asm_ptr_id = codegen_operand(
          ir_ptr_id, ir_context, builder, codegen_context, false, false
        );

        auto asm_value = builder.context.get_operand(asm_value_id);
        auto asm_ptr = builder.context.get_operand(asm_ptr_id);

        bool is_float = false;

        if (auto asm_reg = std::get_if<backend::Register>(&asm_value->kind)) {
          is_float = asm_reg->is_float();
        }

        if (auto asm_reg = std::get_if<backend::VirtualRegister>(&asm_value->kind)) {
          is_float = asm_reg->is_float();
        }

        bool is_global = asm_ptr->is_global();

        if (is_float) {
          if (is_global) {
            auto fsw_instruction = builder.fetch_float_pseudo_store_instruction(
              backend::instruction::FloatPseudoStore::FSW, asm_value_id,
              asm_ptr_id,
              builder.fetch_virtual_register(
                backend::VirtualRegisterKind::General
              )
            );

            builder.append_instruction(fsw_instruction);
          } else {
            auto fsw_instruction = builder.fetch_float_store_instruction(
              backend::instruction::FloatStore::Op::FSW, asm_ptr_id,
              asm_value_id, builder.fetch_immediate(0)
            );
            builder.append_instruction(fsw_instruction);
          }
        } else {
          if (is_global) {
            auto sw_instruction = builder.fetch_pseudo_store_instruction(
              backend::instruction::PseudoStore::SW, asm_value_id, asm_ptr_id,
              builder.fetch_virtual_register(
                backend::VirtualRegisterKind::General
              )
            );
            builder.append_instruction(sw_instruction);
          } else {
            auto sw_instruction = builder.fetch_store_instruction(
              backend::instruction::Store::Op::SW, asm_ptr_id, asm_value_id,
              builder.fetch_immediate(0)
            );
            builder.append_instruction(sw_instruction);
          }
        }
      },
      [&](ir::instruction::Load& ir_load) {
        auto ir_dst_id = ir_load.dst_id;
        auto ir_ptr_id = ir_load.ptr_id;

        auto ir_dst = ir_context.get_operand(ir_dst_id);

        bool load_address =
          std::holds_alternative<ir::type::Pointer>(*ir_dst->type);

        auto asm_dst_id = codegen_operand(
          ir_dst_id, ir_context, builder, codegen_context, false, false
        );
        auto asm_ptr_id = codegen_operand(
          ir_ptr_id, ir_context, builder, codegen_context, false, false
        );

        auto asm_dst = builder.context.get_operand(asm_dst_id);
        auto asm_ptr = builder.context.get_operand(asm_ptr_id);

        bool is_float = false;
        bool is_global = asm_ptr->is_global();

        if (auto asm_reg = std::get_if<backend::Register>(&asm_dst->kind)) {
          is_float = asm_reg->is_float();
        }

        if (auto asm_reg = std::get_if<backend::VirtualRegister>(&asm_dst->kind)) {
          is_float = asm_reg->is_float();
        }

        if (is_float) {
          if (is_global) {
            auto flw_instruction = builder.fetch_float_pseudo_load_instruction(
              backend::instruction::FloatPseudoLoad::FLW, asm_dst_id,
              asm_ptr_id,
              builder.fetch_virtual_register(
                backend::VirtualRegisterKind::General
              )
            );
            builder.append_instruction(flw_instruction);
          } else {
            auto flw_instruction = builder.fetch_float_load_instruction(
              backend::instruction::FloatLoad::Op::FLW, asm_dst_id, asm_ptr_id,
              builder.fetch_immediate(0)
            );
            builder.append_instruction(flw_instruction);
          }
        } else if (load_address && is_global) {
          auto la_instruction = builder.fetch_pseudo_load_instruction(
            backend::instruction::PseudoLoad::LA, asm_dst_id, asm_ptr_id
          );
          builder.append_instruction(la_instruction);
        } else {
          if (is_global) {
            auto lw_instruction = builder.fetch_pseudo_load_instruction(
              backend::instruction::PseudoLoad::LW, asm_dst_id, asm_ptr_id
            );
            builder.append_instruction(lw_instruction);
          } else {
            auto lw_instruction = builder.fetch_load_instruction(
              backend::instruction::Load::Op::LW, asm_dst_id, asm_ptr_id,
              builder.fetch_immediate(0)
            );
            builder.append_instruction(lw_instruction);
          }
        }
      },
      [&](ir::instruction::Binary& ir_binary) {
        auto ir_op = ir_binary.op;

        auto ir_dst_id = ir_binary.dst_id;
        auto ir_lhs_id = ir_binary.lhs_id;
        auto ir_rhs_id = ir_binary.rhs_id;

        auto asm_dst_id = codegen_operand(
          ir_dst_id, ir_context, builder, codegen_context, false, false
        );

        auto asm_lhs_id = codegen_operand(
          ir_lhs_id, ir_context, builder, codegen_context, false, true
        );

        switch (ir_op) {
          case ir::instruction::BinaryOp::Add: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, true, true
            );

            bool is_rhs_imm =
              builder.context.get_operand(asm_rhs_id)->is_immediate();

            if (is_rhs_imm) {
              auto addiw_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::ADDIW, asm_dst_id,
                asm_lhs_id, asm_rhs_id
              );
              builder.append_instruction(addiw_instruction);
            } else {
              auto addw_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::ADDW, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );
              builder.append_instruction(addw_instruction);
            }
            break;
          }
          case ir::instruction::BinaryOp::Sub: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto subw_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SUBW, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(subw_instruction);
            break;
          }
          case ir::instruction::BinaryOp::Mul: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto mul_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::MUL, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(mul_instruction);
            break;
          }
          case ir::instruction::BinaryOp::SDiv: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto div_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::DIV, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(div_instruction);
            break;
          }
          case ir::instruction::BinaryOp::SRem: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto rem_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::REM, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(rem_instruction);

            break;
          }
          case ir::instruction::BinaryOp::FAdd: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto fadds_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FADD,
              backend::instruction::FloatBinary::Fmt::S, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(fadds_instruction);

            break;
          }
          case ir::instruction::BinaryOp::FSub: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto fsubs_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FSUB,
              backend::instruction::FloatBinary::Fmt::S, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(fsubs_instruction);

            break;
          }
          case ir::instruction::BinaryOp::FMul: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto fmuls_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FMUL,
              backend::instruction::FloatBinary::Fmt::S, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(fmuls_instruction);

            break;
          }
          case ir::instruction::BinaryOp::FDiv: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto fdivs_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FDIV,
              backend::instruction::FloatBinary::Fmt::S, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(fdivs_instruction);

            break;
          }
        }
      },
      [&](ir::instruction::ICmp& icmp) {
        auto cond = icmp.cond;
        auto asm_dst_id = codegen_operand(
          icmp.dst_id, ir_context, builder, codegen_context, false, false
        );

        switch (cond) {
          case ir::instruction::ICmpCond::Eq: {
            auto asm_lhs_id = codegen_operand(
              icmp.lhs_id, ir_context, builder, codegen_context, false, false
            );
            auto asm_rhs_id = codegen_operand(
              icmp.rhs_id, ir_context, builder, codegen_context, false, false
            );

            auto asm_tmp_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );

            auto asm_rhs = builder.context.get_operand(asm_rhs_id);

            auto sub_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SUB, asm_tmp_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(sub_instruction);

            // Pseudo seqz
            auto sltiu_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::SLTIU, asm_dst_id, asm_tmp_id,
              builder.fetch_immediate(1)
            );

            builder.append_instruction(sltiu_instruction);

            break;
          }
          case ir::instruction::ICmpCond::Ne: {
            auto asm_lhs_id = codegen_operand(
              icmp.lhs_id, ir_context, builder, codegen_context, false, false
            );
            auto asm_rhs_id = codegen_operand(
              icmp.rhs_id, ir_context, builder, codegen_context, false, false
            );

            auto asm_tmp_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );

            auto asm_rhs = builder.context.get_operand(asm_rhs_id);

            auto sub_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SUB, asm_tmp_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(sub_instruction);

            // Pseudo snez
            auto sltu_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SLTU, asm_dst_id,
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Zero}),
              asm_tmp_id
            );

            builder.append_instruction(sltu_instruction);

            break;
          }
          case ir::instruction::ICmpCond::Slt: {
            auto asm_lhs_id = codegen_operand(
              icmp.lhs_id, ir_context, builder, codegen_context, false, false
            );
            auto asm_rhs_id = codegen_operand(
              icmp.rhs_id, ir_context, builder, codegen_context, true, false
            );

            auto asm_rhs = builder.context.get_operand(asm_rhs_id);

            if (asm_rhs->is_immediate()) {
              auto slti_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::SLTI, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );

              builder.append_instruction(slti_instruction);
            } else {
              auto slt_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::SLT, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );

              builder.append_instruction(slt_instruction);
            }

            break;
          }
          case ir::instruction::ICmpCond::Sle: {
            // lhs <= rhs <=> !(lhs > rhs) <=> !(rhs < lhs)
            auto asm_lhs_id = codegen_operand(
              icmp.lhs_id, ir_context, builder, codegen_context, false, false
            );
            auto asm_rhs_id = codegen_operand(
              icmp.rhs_id, ir_context, builder, codegen_context, true, false
            );
            auto asm_tmp_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );

            auto asm_rhs = builder.context.get_operand(asm_rhs_id);

            if (asm_rhs->is_immediate()) {
              auto slti_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::SLTI, asm_tmp_id, asm_rhs_id,
                asm_lhs_id
              );

              builder.append_instruction(slti_instruction);
            } else {
              auto slt_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::SLT, asm_tmp_id, asm_rhs_id,
                asm_lhs_id
              );

              builder.append_instruction(slt_instruction);
            }

            // Pseudo not
            auto xori_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::XORI, asm_dst_id, asm_tmp_id,
              builder.fetch_immediate(-1)
            );

            builder.append_instruction(xori_instruction);

            break;
          }
        }
      },
      [&](ir::instruction::FCmp& fcmp) {
        auto cond = fcmp.cond;
        auto asm_dst_id = codegen_operand(
          fcmp.dst_id, ir_context, builder, codegen_context, false, false
        );
        auto asm_lhs_id = codegen_operand(
          fcmp.lhs_id, ir_context, builder, codegen_context, false, true
        );
        auto asm_rhs_id = codegen_operand(
          fcmp.rhs_id, ir_context, builder, codegen_context, false, true
        );
        switch (cond) {
          case ir::instruction::FCmpCond::Oeq: {
            auto feqs_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FEQ,
              backend::instruction::FloatBinary::Fmt::S, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(feqs_instruction);
            break;
          }
          case ir::instruction::FCmpCond::One: {
            auto asm_tmp_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );
            auto feqs_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FEQ,
              backend::instruction::FloatBinary::Fmt::S, asm_tmp_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(feqs_instruction);

            // Pseudo not
            auto xori_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::XORI, asm_dst_id, asm_tmp_id,
              builder.fetch_immediate(-1)
            );

            builder.append_instruction(xori_instruction);

            break;
          }
          case ir::instruction::FCmpCond::Olt: {
            auto flts_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FLT,
              backend::instruction::FloatBinary::Fmt::S, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(flts_instruction);

            break;
          }
          case ir::instruction::FCmpCond::Ole: {
            auto fles_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FLE,
              backend::instruction::FloatBinary::Fmt::S, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(fles_instruction);

            break;
          }
        }
      },
      [&](ir::instruction::Cast& cast) {
        auto op = cast.op;
        auto asm_dst = codegen_operand(
          cast.dst_id, ir_context, builder, codegen_context, false, false
        );
        auto asm_src = codegen_operand(
          cast.src_id, ir_context, builder, codegen_context, false, false
        );

        switch (op) {
          case ir::instruction::CastOp::ZExt: {
            // Just move
            auto addi_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::Op::ADDI, asm_dst, asm_src,
              builder.fetch_immediate(0)
            );
            builder.append_instruction(addi_instruction);
            break;
          }
          case ir::instruction::CastOp::BitCast: {
            auto addi_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::Op::ADDI, asm_dst, asm_src,
              builder.fetch_immediate(0)
            );
            builder.append_instruction(addi_instruction);
            break;
          }
          case ir::instruction::CastOp::FPToSI: {
            auto fcvtws_instruction = builder.fetch_float_convert_instruction(
              backend::instruction::FloatConvert::Fmt::W,
              backend::instruction::FloatConvert::Fmt::S, asm_dst, asm_src
            );
            builder.append_instruction(fcvtws_instruction);
            break;
          }
          case ir::instruction::CastOp::SIToFP: {
            auto fcvtsw_instruction = builder.fetch_float_convert_instruction(
              backend::instruction::FloatConvert::Fmt::S,
              backend::instruction::FloatConvert::Fmt::W, asm_dst, asm_src
            );
            builder.append_instruction(fcvtsw_instruction);
            break;
          }
        }
      },
      [&](ir::instruction::Br& br) {
        auto saved_curr_basic_block = builder.curr_basic_block;
        auto ir_block_id = br.block_id;

        auto ir_basic_block = ir_context.get_basic_block(ir_block_id);

        codegen_basic_block(ir_basic_block, ir_context, builder,
                            codegen_context);

        builder.set_curr_basic_block(saved_curr_basic_block);
        
        auto asm_block_id = codegen_context.basic_block_map.at(ir_block_id);
        
        auto j_instruction = builder.fetch_j_instruction(asm_block_id);

        builder.append_instruction(j_instruction);
      },
      [&](ir::instruction::CondBr& condbr) {
        auto saved_curr_basic_block = builder.curr_basic_block;

        auto ir_then_block_id = condbr.then_block_id;
        auto ir_else_block_id = condbr.else_block_id;

        auto ir_then_basic_block = ir_context.get_basic_block(ir_then_block_id);
        auto ir_else_basic_block = ir_context.get_basic_block(ir_else_block_id);

        codegen_basic_block(ir_else_basic_block, ir_context, builder,
                            codegen_context);
        codegen_basic_block(ir_then_basic_block, ir_context, builder,
                            codegen_context);

        builder.set_curr_basic_block(saved_curr_basic_block);

        auto asm_cond_id = codegen_operand(
          condbr.cond_id, ir_context, builder, codegen_context, false, false
        );

        auto bnez_instruction = builder.fetch_branch_instruction(
          backend::instruction::Branch::BNE,
          asm_cond_id,
          builder.fetch_register(backend::Register{
            backend::GeneralRegister::Zero}),
          codegen_context.basic_block_map.at(ir_then_block_id)
        );

        builder.append_instruction(bnez_instruction);

        auto j_instruction = builder.fetch_j_instruction(
          codegen_context.basic_block_map.at(ir_else_block_id)
        );

        builder.append_instruction(j_instruction);
      },
      [&](ir::instruction::Phi& phi) {
        // Phi instruction should be eliminated in SSA construction.
      },
      [&](ir::instruction::Call& call) {
        // TODO
      },
      [&](ir::instruction::GetElementPtr& gep) {
        // TODO
      },
      [&](ir::instruction::Ret& ret) {
        auto ir_maybe_value_id = ret.maybe_value_id;
        if (ir_maybe_value_id.has_value()) {
          auto asm_value_id = codegen_operand(
            ir_maybe_value_id.value(), ir_context, builder, codegen_context,
            false, false
          );
          auto a0_id = builder.fetch_register(backend::Register{
            backend::GeneralRegister::A0});

          auto mv_instruction = builder.fetch_binary_imm_instruction(
            backend::instruction::BinaryImm::Op::ADDI, a0_id, asm_value_id,
            builder.fetch_immediate(0)
          );

          builder.append_instruction(mv_instruction);
        }
        auto ret_instruction = builder.fetch_ret_instruction();
        builder.append_instruction(ret_instruction);
      },
      [&](auto& ir_instruction) {

      },
    },
    ir_instruction->kind
  );
}

AsmOperandID codegen_operand(
  IrOperandID ir_operand_id,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context,
  bool try_keep_imm,
  bool use_fmv
) {
  auto ir_operand = ir_context.get_operand(ir_operand_id);
  auto& ir_operand_kind = ir_operand->kind;
  auto ir_operand_type = ir_operand->type;

  bool is_float = std::holds_alternative<ir::type::Float>(*ir_operand_type);

  AsmOperandID asm_operand_id;

  std::visit(
    overloaded{
      [&](ir::operand::Arbitrary& _) {
        // Using virtual register for arbitrary operand in ir.
        // This is only for the operand in `dst` position.
        AsmOperandID asm_temp_id;

        if (codegen_context.operand_map.find(ir_operand_id) !=
            codegen_context.operand_map.end()) {
          asm_temp_id = codegen_context.operand_map.at(ir_operand_id);

          auto asm_temp = builder.context.get_operand(asm_temp_id);

          if (asm_temp->is_local_memory()) {
            int offset = std::get<backend::LocalMemory>(asm_temp->kind).offset;

            auto asm_sp_id = builder.fetch_register(backend::Register{
              backend::GeneralRegister::Sp});

            asm_operand_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );

            if (check_itype_immediate(offset)) {
              auto addi_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::ADDI, asm_operand_id,
                asm_sp_id, builder.fetch_immediate(offset)
              );
              builder.append_instruction(addi_instruction);
            } else {
              auto asm_imm_reg_id = builder.fetch_virtual_register(
                backend::VirtualRegisterKind::General
              );
              auto li_instruction = builder.fetch_li_instruction(
                asm_imm_reg_id, builder.fetch_immediate(offset)
              );
              builder.append_instruction(li_instruction);
              auto add_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::ADD, asm_operand_id,
                asm_sp_id, asm_imm_reg_id
              );
              builder.append_instruction(add_instruction);
            }
          } else {
            asm_operand_id = asm_temp_id;
          }
        } else if (is_float) {
          asm_operand_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::Float);
        } else {
          asm_operand_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );
        }
        codegen_context.operand_map[ir_operand_id] = asm_operand_id;
      },
      [&](ir::operand::ConstantPtr ir_constant) {
        if (is_float) {
          float value = std::get<float>(ir_constant->kind);
          uint32_t bits = *reinterpret_cast<uint32_t*>(&value);
          auto asm_imm_id = builder.fetch_immediate((uint32_t)bits);

          auto asm_temp_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );

          if (check_utype_immediate(bits)) {
            auto lui_instruction = builder.fetch_lui_instruction(
              asm_temp_id, builder.fetch_immediate((uint32_t)(bits >> 12))
            );
            builder.append_instruction(lui_instruction);
          } else {
            auto li_instruction =
              builder.fetch_li_instruction(asm_temp_id, asm_imm_id);
            builder.append_instruction(li_instruction);
          }

          if (use_fmv) {
            asm_operand_id =
              builder.fetch_virtual_register(backend::VirtualRegisterKind::Float
              );
            auto fmv_instruction = builder.fetch_float_move_instruction(
              backend::instruction::FloatMove::Fmt::S,
              backend::instruction::FloatMove::Fmt::X, asm_operand_id,
              asm_temp_id
            );
            builder.append_instruction(fmv_instruction);
          } else {
            asm_operand_id = asm_temp_id;
          }
        } else {
          int value = std::get<int>(ir_constant->kind);
          auto asm_imm_id = builder.fetch_immediate((int32_t)value);

          if (check_itype_immediate(value) && try_keep_imm) {
            asm_operand_id = asm_imm_id;
          } else {
            asm_operand_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );
            uint32_t bits = *reinterpret_cast<uint32_t*>(&value);
            if (check_utype_immediate(bits)) {
              auto lui_instruction =
                builder.fetch_lui_instruction(asm_operand_id, asm_imm_id);
              builder.append_instruction(lui_instruction);
            } else {
              auto li_instruction =
                builder.fetch_li_instruction(asm_operand_id, asm_imm_id);
              builder.append_instruction(li_instruction);
            }
          }
        }
      },
      [&](ir::operand::Global& ir_global) {
        if (codegen_context.operand_map.find(ir_operand_id) == 
            codegen_context.operand_map.end()) {
          throw std::runtime_error("global variable is not initialized.");
        }
        asm_operand_id = codegen_context.operand_map.at(ir_operand_id);
      },
      [](auto& k) {
        // TODO: Parameter
        throw std::runtime_error("Invalid operand kind.");
      },
    },
    ir_operand_kind
  );

  return asm_operand_id;
}

bool check_utype_immediate(uint32_t value) {
  return ((value & 0xfff) == 0);
}

bool check_itype_immediate(int32_t value) {
  return (value >= -0x800 && value <= 0x7ff);
}

/// Perform register allocation.
void asm_register_allocation(AsmBuilder& builder) {
  for (auto& [function_name, function] : builder.context.function_table) {
    auto linear_scan_context = backend::LinearScanContext();
    backend::linear_scan(function, builder, linear_scan_context);
  }
}

/// Perform instruction scheduling.
void asm_instruction_scheduling(AsmContext& context) {
  // TODO
}

}  // namespace syc
