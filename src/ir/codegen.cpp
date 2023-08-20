#include "ir/codegen.h"
#include "passes/asm/greedy_allocation.h"

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

    if (ir_init_constant->type->as<ir::type::Integer>().has_value()) {
      int value = std::get<int>(ir_init_constant_kind);
      // bitwise conversion
      std::vector<uint32_t> asm_value = {*reinterpret_cast<uint32_t*>(&value)};
      auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
      codegen_context.operand_map[ir_operand_id] = asm_operand_id;

    } else if (ir_init_constant_type->as<ir::type::Float>().has_value()) {
      float value = std::get<float>(ir_init_constant_kind);
      // bitwise conversion
      std::vector<uint32_t> asm_value = {*reinterpret_cast<uint32_t*>(&value)};
      auto asm_operand_id = builder.fetch_global(ir_global.name, asm_value);
      codegen_context.operand_map[ir_operand_id] = asm_operand_id;

    } else if (ir_init_constant_type->as<ir::type::Array>().has_value()) {
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
            if (ir_constant->type->as<ir::type::Array>().has_value()) {
              auto& ir_constant_kind = ir_constant->kind;
              if (auto ir_constant_list = std::get_if<std::vector<ir::operand::ConstantPtr>>(&ir_constant_kind)) {
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
              if (ir_constant_type->as<ir::type::Integer>().has_value()) {
                int value = std::get<int>(ir_constant->kind);
                // bitwise conversion
                uint32_t asm_value_element =
                  *reinterpret_cast<uint32_t*>(&value);
                asm_value.push_back(asm_value_element);

              } else if (ir_constant_type->as<ir::type::Float>().has_value()) {
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
}

void codegen_rest(
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  asm_register_allocation(builder);

  for (auto& [ir_function_name, ir_function] : ir_context.function_table) {
    if (ir_function->is_declare) {
      continue;
    }
    // adjust stack frame size to be 16-byte aligned
    codegen_function_prolouge(
      ir_function_name, ir_context, builder, codegen_context
    );
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
    auto asm_basic_block = builder.fetch_basic_block();
    builder.curr_function->append_basic_block(asm_basic_block);
    codegen_context.basic_block_map[curr_ir_basic_block->id] =
      asm_basic_block->id;
    curr_ir_basic_block = curr_ir_basic_block->next;
  }

  int curr_general_reg = 0;
  int curr_float_reg = 0;

  std::vector<IrOperandID> ir_stack_param_id_list;

  auto entry_block = builder.curr_function->head_basic_block->next;
  builder.set_curr_basic_block(entry_block);

  for (auto ir_operand_id : ir_function->parameter_id_list) {
    auto ir_operand = ir_context.get_operand(ir_operand_id);
    auto ir_operand_type = ir_operand->type;

    if (ir_operand_type->as<ir::type::Float>().has_value()) {
      if (curr_float_reg <= 7) {
        auto asm_reg =
          builder.fetch_register(backend::Register{(backend::FloatRegister)(
            (int)backend::FloatRegister::Fa0 + curr_float_reg
          )});
        auto vreg =
          builder.fetch_virtual_register(backend::VirtualRegisterKind::Float);
        auto fsgnjs_instruction = builder.fetch_float_binary_instruction(
          backend::instruction::FloatBinary::FSGNJ,
          backend::instruction::FloatBinary::S, vreg, asm_reg, asm_reg
        );
        builder.append_instruction(fsgnjs_instruction);
        codegen_context.operand_map[ir_operand_id] = vreg;
        curr_float_reg++;
      } else {
        ir_stack_param_id_list.push_back(ir_operand_id);
      }
    } else {
      if (curr_general_reg <= 7) {
        auto asm_reg =
          builder.fetch_register(backend::Register{(backend::GeneralRegister)(
            (int)backend::GeneralRegister::A0 + curr_general_reg
          )});
        auto vreg =
          builder.fetch_virtual_register(backend::VirtualRegisterKind::General);
        auto addi_instruction = builder.fetch_binary_imm_instruction(
          backend::instruction::BinaryImm::ADDI, vreg, asm_reg,
          builder.fetch_immediate(0)
        );
        builder.append_instruction(addi_instruction);
        codegen_context.operand_map[ir_operand_id] = vreg;
        curr_general_reg++;
      } else {
        ir_stack_param_id_list.push_back(ir_operand_id);
      }
    }
  }
  int offset = 0;
  for (auto ir_operand_id : ir_stack_param_id_list) {
    auto asm_local_memory_id = builder.fetch_local_memory(
      offset, backend::Register{backend::GeneralRegister::S0}
    );
    codegen_context.operand_map[ir_operand_id] = asm_local_memory_id;
    offset += 8;
  }

  curr_ir_basic_block = ir_function->head_basic_block->next;
  while (curr_ir_basic_block != ir_function->tail_basic_block) {
    codegen_basic_block(
      curr_ir_basic_block, ir_context, builder, codegen_context
    );
    curr_ir_basic_block = curr_ir_basic_block->next;
  }
}

void codegen_function_prolouge(
  std::string function_name,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  using namespace backend;

  auto ir_function = ir_context.get_function(function_name);

  auto asm_function = builder.context.get_function(function_name);
  asm_function->add_saved_register(Register{GeneralRegister::S0});

  auto entry_block = asm_function->head_basic_block->next;

  auto stack_frame_size = asm_function->stack_frame_size;

  // store ra
  auto ra_id = builder.fetch_register(Register{GeneralRegister::Ra});

  auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

  stack_frame_size += 8 * (1 + asm_function->saved_register_set.size());

  asm_function->stack_frame_size = stack_frame_size;

  // Adjust frame size to be 16-byte aligned
  // There might be gaps between saved registers and local variables.
  size_t aligned_stack_frame_size = (stack_frame_size + 15) / 16 * 16;

  asm_function->align_frame_size =
    aligned_stack_frame_size - asm_function->stack_frame_size;

  // addi s0, sp, aligned_frame_size
  // this is for parameters that are passed through stack
  if (check_itype_immediate(aligned_stack_frame_size)) {
    auto addi_s0_instruction = builder.fetch_binary_imm_instruction(
      instruction::BinaryImm::Op::ADDI,
      builder.fetch_register(Register{GeneralRegister::S0}), sp_id,
      builder.fetch_immediate((int32_t)aligned_stack_frame_size)
    );

    entry_block->prepend_instruction(addi_s0_instruction);

  } else {
    auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
    auto li_instruction = builder.fetch_li_instruction(
      asm_tmp_id, builder.fetch_immediate((int32_t)aligned_stack_frame_size)
    );

    auto add_instruction = builder.fetch_binary_instruction(
      instruction::Binary::Op::ADD,
      builder.fetch_register(Register{GeneralRegister::S0}), sp_id, asm_tmp_id
    );

    entry_block->prepend_instruction(add_instruction);
    entry_block->prepend_instruction(li_instruction);
  }

  size_t curr_frame_pos = aligned_stack_frame_size - 16;

  for (auto reg : asm_function->saved_register_set) {
    auto reg_id = builder.fetch_register(reg);

    if (reg.is_general()) {
      if (check_itype_immediate((int32_t)curr_frame_pos)) {
        auto sd_instruction = builder.fetch_store_instruction(
          instruction::Store::Op::SD, sp_id, reg_id,
          builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        entry_block->prepend_instruction(sd_instruction);
      } else {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
        auto li_instruction = builder.fetch_li_instruction(
          asm_tmp_id, builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id
        );
        auto sd_instruction = builder.fetch_store_instruction(
          instruction::Store::Op::SD, asm_tmp_id, reg_id,
          builder.fetch_immediate(0)
        );

        entry_block->prepend_instruction(sd_instruction);
        entry_block->prepend_instruction(add_instruction);
        entry_block->prepend_instruction(li_instruction);
      }
    } else {
      if (check_itype_immediate((int32_t)curr_frame_pos)) {
        auto fsd_instruction = builder.fetch_float_store_instruction(
          instruction::FloatStore::Op::FSD, sp_id, reg_id,
          builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        entry_block->prepend_instruction(fsd_instruction);
      } else {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
        auto li_instruction = builder.fetch_li_instruction(
          asm_tmp_id, builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id
        );
        auto fsd_instruction = builder.fetch_float_store_instruction(
          instruction::FloatStore::Op::FSD, asm_tmp_id, reg_id,
          builder.fetch_immediate(0)
        );

        entry_block->prepend_instruction(fsd_instruction);
        entry_block->prepend_instruction(add_instruction);
        entry_block->prepend_instruction(li_instruction);
      }
    }

    curr_frame_pos -= 8;
  }

  if (check_itype_immediate((int32_t)(aligned_stack_frame_size - 8))) {
    auto sd_instruction = builder.fetch_store_instruction(
      instruction::Store::Op::SD, sp_id, ra_id,
      builder.fetch_immediate((int32_t)(aligned_stack_frame_size - 8))
    );
    entry_block->prepend_instruction(sd_instruction);
  } else {
    auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
    auto li_instruction = builder.fetch_li_instruction(
      asm_tmp_id,
      builder.fetch_immediate((int32_t)(aligned_stack_frame_size - 8))
    );
    auto add_instruction = builder.fetch_binary_instruction(
      instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id
    );
    auto sd_instruction = builder.fetch_store_instruction(
      instruction::Store::Op::SD, asm_tmp_id, ra_id, builder.fetch_immediate(0)
    );

    entry_block->prepend_instruction(sd_instruction);
    entry_block->prepend_instruction(add_instruction);
    entry_block->prepend_instruction(li_instruction);
  }

  if (check_itype_immediate(-(int32_t)aligned_stack_frame_size)) {
    auto addi_instruction = builder.fetch_binary_imm_instruction(
      instruction::BinaryImm::Op::ADDI, sp_id, sp_id,
      builder.fetch_immediate(-(int32_t)aligned_stack_frame_size)
    );
    entry_block->prepend_instruction(addi_instruction);
  } else {
    auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
    auto li_instruction = builder.fetch_li_instruction(
      asm_tmp_id, builder.fetch_immediate(-(int32_t)aligned_stack_frame_size)
    );

    auto add_instruction = builder.fetch_binary_instruction(
      instruction::Binary::Op::ADD, sp_id, sp_id, asm_tmp_id
    );

    entry_block->prepend_instruction(add_instruction);
    entry_block->prepend_instruction(li_instruction);
  }
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

  size_t aligned_stack_frame_size =
    asm_function->stack_frame_size + asm_function->align_frame_size;
  size_t curr_frame_pos = aligned_stack_frame_size - 16;

  for (auto reg : asm_function->saved_register_set) {
    auto reg_id = builder.fetch_register(reg);

    if (reg.is_general()) {
      if (check_itype_immediate((int32_t)(curr_frame_pos))) {
        auto ld_instruction = builder.fetch_load_instruction(
          instruction::Load::Op::LD, reg_id, sp_id,
          builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        last_instruction->insert_prev(ld_instruction);
      } else {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
        auto li_instruction = builder.fetch_li_instruction(
          asm_tmp_id, builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id
        );
        auto ld_instruction = builder.fetch_load_instruction(
          instruction::Load::Op::LD, reg_id, asm_tmp_id,
          builder.fetch_immediate(0)
        );

        last_instruction->insert_prev(li_instruction);
        last_instruction->insert_prev(add_instruction);
        last_instruction->insert_prev(ld_instruction);
      }
    } else {
      if (check_itype_immediate((int32_t)(curr_frame_pos))) {
        auto fsd_instruction = builder.fetch_float_load_instruction(
          instruction::FloatLoad::Op::FLD, reg_id, sp_id,
          builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        last_instruction->insert_prev(fsd_instruction);
      } else {
        auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
        auto li_instruction = builder.fetch_li_instruction(
          asm_tmp_id, builder.fetch_immediate((int32_t)curr_frame_pos)
        );
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id
        );
        auto fsd_instruction = builder.fetch_float_load_instruction(
          instruction::FloatLoad::Op::FLD, reg_id, asm_tmp_id,
          builder.fetch_immediate(0)
        );

        last_instruction->insert_prev(li_instruction);
        last_instruction->insert_prev(add_instruction);
        last_instruction->insert_prev(fsd_instruction);
      }
    }
    curr_frame_pos -= 8;
  }

  if (check_itype_immediate((int32_t)(aligned_stack_frame_size - 8))) {
    auto ld_instruction = builder.fetch_load_instruction(
      instruction::Load::Op::LD, ra_id, sp_id,
      builder.fetch_immediate((int32_t)(aligned_stack_frame_size - 8))
    );
    last_instruction->insert_prev(ld_instruction);
  } else {
    auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
    auto li_instruction = builder.fetch_li_instruction(
      asm_tmp_id,
      builder.fetch_immediate((int32_t)(aligned_stack_frame_size - 8))
    );
    auto add_instruction = builder.fetch_binary_instruction(
      instruction::Binary::Op::ADD, asm_tmp_id, sp_id, asm_tmp_id
    );
    auto ld_instruction = builder.fetch_load_instruction(
      instruction::Load::Op::LD, ra_id, asm_tmp_id, builder.fetch_immediate(0)
    );

    last_instruction->insert_prev(li_instruction);
    last_instruction->insert_prev(add_instruction);
    last_instruction->insert_prev(ld_instruction);
  }

  if (check_itype_immediate((int32_t)(stack_frame_size + align_frame_size))) {
    auto addi_instruction = builder.fetch_binary_imm_instruction(
      instruction::BinaryImm::Op::ADDI, sp_id, sp_id,
      builder.fetch_immediate((int32_t)(stack_frame_size + align_frame_size))
    );

    last_instruction->insert_prev(addi_instruction);
  } else {
    auto asm_tmp_id = builder.fetch_register(Register{GeneralRegister::T2});
    auto li_instruction = builder.fetch_li_instruction(
      asm_tmp_id,
      builder.fetch_immediate((int32_t)(stack_frame_size + align_frame_size))
    );

    auto add_instruction = builder.fetch_binary_instruction(
      instruction::Binary::Op::ADD, sp_id, sp_id, asm_tmp_id
    );

    last_instruction->insert_prev(li_instruction);
    last_instruction->insert_prev(add_instruction);
  }
}

void codegen_basic_block(
  IrBasicBlockPtr ir_basic_block,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  auto asm_block_id = codegen_context.basic_block_map[ir_basic_block->id];
  auto basic_block = builder.context.get_basic_block(asm_block_id);
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

        if (ir_allocated_size == 1) {
          ir_allocated_size = 32;
        }

        auto asm_allocated_size = ir_allocated_size / 8;
        auto asm_local_memory_id = builder.fetch_local_memory(
          curr_frame_size, backend::Register{backend::GeneralRegister::Sp}
        );
        builder.curr_function->stack_frame_size += asm_allocated_size;

        codegen_context.operand_map[ir_alloca.dst_id] = asm_local_memory_id;
      },
      [&](ir::instruction::Store& ir_store) {
        auto ir_value_id = ir_store.value_id;
        auto ir_ptr_id = ir_store.ptr_id;

        auto ir_value = ir_context.get_operand(ir_value_id);
        auto ir_ptr = ir_context.get_operand(ir_ptr_id);

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
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::T2})
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
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::T2})
            );
            builder.append_instruction(sw_instruction);
          } else {
            if (ir::get_size(ir_value->type) == 64) {
              auto sd_instruction = builder.fetch_store_instruction(
                backend::instruction::Store::Op::SD, asm_ptr_id, asm_value_id,
                builder.fetch_immediate(0)
              );
              builder.append_instruction(sd_instruction);
            } else {
              auto sw_instruction = builder.fetch_store_instruction(
                backend::instruction::Store::Op::SW, asm_ptr_id, asm_value_id,
                builder.fetch_immediate(0)
              );
              builder.append_instruction(sw_instruction);
            }
          }
        }
      },
      [&](ir::instruction::Load& ir_load) {
        auto ir_dst_id = ir_load.dst_id;
        auto ir_ptr_id = ir_load.ptr_id;

        auto ir_dst = ir_context.get_operand(ir_dst_id);

        bool load_address = ir_dst->type->as<ir::type::Pointer>().has_value();

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
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::T2})
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
            if (ir::get_size(ir_dst->type) == 64) {
              auto ld_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LD, asm_dst_id, asm_ptr_id,
                builder.fetch_immediate(0)
              );
              builder.append_instruction(ld_instruction);
            } else {
              auto lw_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LW, asm_dst_id, asm_ptr_id,
                builder.fetch_immediate(0)
              );
              builder.append_instruction(lw_instruction);
            }
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

            auto op = backend::instruction::Binary::Op::MULW;
            if (ir_binary.indvar_overflow_hint) {
              op = backend::instruction::Binary::Op::MUL;
            }

            auto mul_instruction = builder.fetch_binary_instruction(
              op, asm_dst_id, asm_lhs_id, asm_rhs_id
            );
            builder.append_instruction(mul_instruction);
            break;
          }
          case ir::instruction::BinaryOp::SDiv: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto div_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::DIVW, asm_dst_id, asm_lhs_id,
              asm_rhs_id
            );
            builder.append_instruction(div_instruction);
            break;
          }
          case ir::instruction::BinaryOp::SRem: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, false, true
            );

            auto op = backend::instruction::Binary::Op::REMW;
            if (ir_binary.indvar_overflow_hint) {
              op = backend::instruction::Binary::Op::REM;
            }

            auto rem_instruction = builder.fetch_binary_instruction(
              op, asm_dst_id, asm_lhs_id, asm_rhs_id
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
          case ir::instruction::BinaryOp::Shl: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, true, true
            );

            bool is_rhs_imm =
              builder.context.get_operand(asm_rhs_id)->is_immediate();

            if (is_rhs_imm) {
              auto slli_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::SLLIW, asm_dst_id,
                asm_lhs_id, asm_rhs_id
              );
              builder.append_instruction(slli_instruction);
            } else {
              auto sll_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::SLLW, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );
              builder.append_instruction(sll_instruction);
            }
            break;
          }
          case ir::instruction::BinaryOp::LShr: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, true, true
            );

            bool is_rhs_imm =
              builder.context.get_operand(asm_rhs_id)->is_immediate();

            if (is_rhs_imm) {
              auto srli_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::SRLIW, asm_dst_id,
                asm_lhs_id, asm_rhs_id
              );
              builder.append_instruction(srli_instruction);
            } else {
              auto srl_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::SRLW, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );
              builder.append_instruction(srl_instruction);
            }
            break;
          }
          case ir::instruction::BinaryOp::AShr: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, true, true
            );

            bool is_rhs_imm =
              builder.context.get_operand(asm_rhs_id)->is_immediate();

            if (is_rhs_imm) {
              auto srai_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::SRAIW, asm_dst_id,
                asm_lhs_id, asm_rhs_id
              );
              builder.append_instruction(srai_instruction);
            } else {
              auto sra_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::SRAW, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );
              builder.append_instruction(sra_instruction);
            }
            break;
          }
          case ir::instruction::BinaryOp::And: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, true, false
            );

            bool is_rhs_imm =
              builder.context.get_operand(asm_rhs_id)->is_immediate();
            if (is_rhs_imm) {
              auto andi_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::ANDI, asm_dst_id,
                asm_lhs_id, asm_rhs_id
              );
              builder.append_instruction(andi_instruction);
            } else {
              auto and_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::AND, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );
              builder.append_instruction(and_instruction);
            }
            break;
          }
          case ir::instruction::BinaryOp::Or: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, true, false
            );

            bool is_rhs_imm =
              builder.context.get_operand(asm_rhs_id)->is_immediate();
            if (is_rhs_imm) {
              auto ori_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::ORI, asm_dst_id,
                asm_lhs_id, asm_rhs_id
              );
              builder.append_instruction(ori_instruction);
            } else {
              auto or_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::OR, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );
              builder.append_instruction(or_instruction);
            }
            break;
          }
          case ir::instruction::BinaryOp::Xor: {
            auto asm_rhs_id = codegen_operand(
              ir_rhs_id, ir_context, builder, codegen_context, true, false
            );

            bool is_rhs_imm =
              builder.context.get_operand(asm_rhs_id)->is_immediate();
            if (is_rhs_imm) {
              auto xori_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::XORI, asm_dst_id,
                asm_lhs_id, asm_rhs_id
              );
              builder.append_instruction(xori_instruction);
            } else {
              auto xor_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::XOR, asm_dst_id, asm_lhs_id,
                asm_rhs_id
              );
              builder.append_instruction(xor_instruction);
            }

            break;
          }
          default: {
            throw std::runtime_error("unimplemented binary op");
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

            auto tmp_reg_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );

            auto asm_rhs = builder.context.get_operand(asm_rhs_id);

            auto sub_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SUB, tmp_reg_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(sub_instruction);

            // Pseudo seqz
            auto sltiu_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::SLTIU, asm_dst_id, tmp_reg_id,
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

            auto tmp_reg_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );

            auto asm_rhs = builder.context.get_operand(asm_rhs_id);

            auto sub_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SUB, tmp_reg_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(sub_instruction);

            // Pseudo snez
            auto sltu_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SLTU, asm_dst_id,
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Zero}),
              tmp_reg_id
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
              icmp.rhs_id, ir_context, builder, codegen_context, false, false
            );
            auto tmp_reg_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );

            auto slt_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::SLT, tmp_reg_id, asm_rhs_id,
              asm_lhs_id
            );

            builder.append_instruction(slt_instruction);

            // Pseudo seqz
            auto sltiu_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::SLTIU, asm_dst_id, tmp_reg_id,
              builder.fetch_immediate(1)
            );

            builder.append_instruction(sltiu_instruction);

            break;
          }
          default: {
            throw std::runtime_error("Unimplemented ICmpCond");
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
            auto tmp_reg_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );
            auto feqs_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FEQ,
              backend::instruction::FloatBinary::Fmt::S, tmp_reg_id, asm_lhs_id,
              asm_rhs_id
            );

            builder.append_instruction(feqs_instruction);

            // Pseudo seqz
            auto sltiu_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::SLTIU, asm_dst_id, tmp_reg_id,
              builder.fetch_immediate(1)
            );

            builder.append_instruction(sltiu_instruction);

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
          cast.src_id, ir_context, builder, codegen_context, false, true
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
            auto asm_src_operand = builder.context.get_operand(asm_src);
            if (asm_src_operand->is_global()) {
              auto la_instuction = builder.fetch_pseudo_load_instruction(
                backend::instruction::PseudoLoad::LA, asm_dst, asm_src
              );
              builder.append_instruction(la_instuction);
            } else {
              auto addi_instruction = builder.fetch_binary_imm_instruction(
                backend::instruction::BinaryImm::Op::ADDI, asm_dst, asm_src,
                builder.fetch_immediate(0)
              );
              builder.append_instruction(addi_instruction);
            }
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

        auto asm_cond_id = codegen_operand(
          condbr.cond_id, ir_context, builder, codegen_context, false, false
        );

        auto bnez_instruction = builder.fetch_branch_instruction(
          backend::instruction::Branch::BNE, asm_cond_id,
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
        auto asm_dst_id = codegen_operand(
          phi.dst_id, ir_context, builder, codegen_context, false, false
        );

        std::vector<std::tuple<AsmOperandID, AsmBasicBlockID>>
          asm_incoming_list = {};

        for (auto [ir_operand_id, ir_block_id] : phi.incoming_list) {
          auto ir_operand = ir_context.get_operand(ir_operand_id);

          AsmOperandID asm_operand_id;

          asm_operand_id = codegen_operand(
            ir_operand_id, ir_context, builder, codegen_context, false, false,
            true, true
          );

          auto asm_block_id = codegen_context.basic_block_map.at(ir_block_id);

          asm_incoming_list.push_back({asm_operand_id, asm_block_id});
        }

        auto phi_instruction =
          builder.fetch_phi_instruction(asm_dst_id, asm_incoming_list);

        builder.append_instruction(phi_instruction);
      },
      [&](ir::instruction::Call& call) {
        int curr_general_reg = 0;
        int curr_float_reg = 0;

        std::vector<AsmOperandID> asm_arg_id_list;
        std::vector<AsmOperandID> asm_stack_arg_id_list;

        std::set<backend::Register> used_arg_reg_set;

        for (auto ir_arg_id : call.arg_id_list) {
          auto asm_arg_id = codegen_operand(
            ir_arg_id, ir_context, builder, codegen_context, false, true
          );

          asm_arg_id_list.push_back(asm_arg_id);

          auto asm_arg = builder.context.get_operand(asm_arg_id);

          if (asm_arg->is_float()) {
            if (curr_float_reg <= 7) {
              auto reg = backend::Register{(backend::FloatRegister)(
                (int)(backend::FloatRegister::Fa0) + curr_float_reg
              )};
              used_arg_reg_set.insert(reg);
              auto asm_reg_id = builder.fetch_register(reg);

              // Pseudo fmv.s
              auto fsgnjs_instruction = builder.fetch_float_binary_instruction(
                backend::instruction::FloatBinary::FSGNJ,
                backend::instruction::FloatBinary::S, asm_reg_id, asm_arg_id,
                asm_arg_id
              );

              builder.append_instruction(fsgnjs_instruction);

              curr_float_reg++;
            } else {
              asm_stack_arg_id_list.push_back(asm_arg_id);
            }
          } else {
            if (curr_general_reg <= 7) {
              auto reg = backend::Register{(backend::GeneralRegister)(
                (int)(backend::GeneralRegister::A0) + curr_general_reg
              )};
              used_arg_reg_set.insert(reg);
              auto asm_reg_id = builder.fetch_register(reg);

              if (asm_arg->is_global()) {
                // la
                auto la_instruction = builder.fetch_pseudo_load_instruction(
                  backend::instruction::PseudoLoad::LA, asm_reg_id, asm_arg_id
                );
                builder.append_instruction(la_instruction);
              } else {
                // mv
                auto addi_instruction = builder.fetch_binary_imm_instruction(
                  backend::instruction::BinaryImm::ADDI, asm_reg_id, asm_arg_id,
                  builder.fetch_immediate(0)
                );

                builder.append_instruction(addi_instruction);
              }

              curr_general_reg++;
            } else {
              asm_stack_arg_id_list.push_back(asm_arg_id);
            }
          }
        }

        int stack_size = 8 * asm_stack_arg_id_list.size();
        int align_size = (stack_size + 15) / 16 * 16 - stack_size;

        if (stack_size > 0) {
          auto asm_arg_stack_reg_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );

          if (check_itype_immediate(-(stack_size + align_size))) {
            auto addi_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::ADDI, asm_arg_stack_reg_id,
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp}),
              builder.fetch_immediate(-(stack_size + align_size))
            );

            builder.append_instruction(addi_instruction);
          } else {
            auto asm_tmp_id = builder.fetch_register(backend::Register{
              backend::GeneralRegister::T2});
            auto li_instruction = builder.fetch_li_instruction(
              asm_tmp_id, builder.fetch_immediate(-(stack_size + align_size))
            );
            builder.append_instruction(li_instruction);

            auto add_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::ADD, asm_arg_stack_reg_id,
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp}),
              asm_tmp_id
            );
            builder.append_instruction(add_instruction);
          }

          int offset = 0;

          for (auto asm_arg_id : asm_stack_arg_id_list) {
            auto asm_arg = builder.context.get_operand(asm_arg_id);

            if (asm_arg->is_global()) {
              auto asm_load_dst_id = builder.fetch_virtual_register(
                backend::VirtualRegisterKind::General
              );
              auto la_instruction = builder.fetch_pseudo_load_instruction(
                backend::instruction::PseudoLoad::LA, asm_load_dst_id,
                asm_arg_id
              );
              builder.append_instruction(la_instruction);
              asm_arg_id = asm_load_dst_id;
              asm_arg = builder.context.get_operand(asm_arg_id);
            }

            if (asm_arg->is_float()) {
              if (check_itype_immediate(offset)) {
                auto fsd_instruction = builder.fetch_float_store_instruction(
                  backend::instruction::FloatStore::Op::FSD,
                  asm_arg_stack_reg_id, asm_arg_id,
                  builder.fetch_immediate(offset)
                );

                builder.append_instruction(fsd_instruction);
              } else {
                auto asm_tmp_id = builder.fetch_register(backend::Register{
                  backend::GeneralRegister::T2});
                auto li_instruction = builder.fetch_li_instruction(
                  asm_tmp_id, builder.fetch_immediate(offset)
                );
                builder.append_instruction(li_instruction);

                auto add_instruction = builder.fetch_binary_instruction(
                  backend::instruction::Binary::Op::ADD, asm_tmp_id,
                  asm_arg_stack_reg_id, asm_tmp_id
                );
                builder.append_instruction(add_instruction);

                auto fsd_instruction = builder.fetch_float_store_instruction(
                  backend::instruction::FloatStore::Op::FSD, asm_tmp_id,
                  asm_arg_id, builder.fetch_immediate(0)
                );

                builder.append_instruction(fsd_instruction);
              }

            } else {
              if (check_itype_immediate(offset)) {
                auto sd_instruction = builder.fetch_store_instruction(
                  backend::instruction::Store::Op::SD, asm_arg_stack_reg_id,
                  asm_arg_id, builder.fetch_immediate(offset)
                );

                builder.append_instruction(sd_instruction);
              } else {
                auto asm_tmp_id = builder.fetch_register(backend::Register{
                  backend::GeneralRegister::T2});
                auto li_instruction = builder.fetch_li_instruction(
                  asm_tmp_id, builder.fetch_immediate(offset)
                );
                builder.append_instruction(li_instruction);

                auto add_instruction = builder.fetch_binary_instruction(
                  backend::instruction::Binary::Op::ADD, asm_tmp_id,
                  asm_arg_stack_reg_id, asm_tmp_id
                );
                builder.append_instruction(add_instruction);

                auto sd_instruction = builder.fetch_store_instruction(
                  backend::instruction::Store::Op::SD, asm_tmp_id, asm_arg_id,
                  builder.fetch_immediate(0)
                );

                builder.append_instruction(sd_instruction);
              }
            }

            offset += 8;
          }
          // mv
          auto addi_instruction = builder.fetch_binary_imm_instruction(
            backend::instruction::BinaryImm::ADDI,
            builder.fetch_register(backend::Register{
              backend::GeneralRegister::Sp}),
            asm_arg_stack_reg_id, builder.fetch_immediate(0)
          );
          builder.append_instruction(addi_instruction);
        }

        auto call_instruction =
          builder.fetch_call_instruction(call.function_name, used_arg_reg_set);

        builder.append_instruction(call_instruction);

        if (stack_size > 0) {
          if (check_itype_immediate(stack_size + align_size)) {
            auto addi_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::ADDI,
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp}),
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp}),
              builder.fetch_immediate(stack_size + align_size)
            );

            builder.append_instruction(addi_instruction);
          } else {
            auto asm_tmp_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );
            auto li_instruction = builder.fetch_li_instruction(
              asm_tmp_id, builder.fetch_immediate(stack_size + align_size)
            );
            builder.append_instruction(li_instruction);

            auto add_instruction = builder.fetch_binary_instruction(
              backend::instruction::Binary::Op::ADD,
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp}),
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::Sp}),
              asm_tmp_id
            );
            builder.append_instruction(add_instruction);
          }
        }

        if (call.maybe_dst_id.has_value()) {
          auto asm_dst_id = codegen_operand(
            call.maybe_dst_id.value(), ir_context, builder, codegen_context,
            false, false
          );

          auto asm_dst = builder.context.get_operand(asm_dst_id);

          if (asm_dst->is_float()) {
            // Pseudo fmv.s
            auto fsgnjs_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FSGNJ,
              backend::instruction::FloatBinary::S, asm_dst_id,
              builder.fetch_register(backend::Register{
                backend::FloatRegister::Fa0}),
              builder.fetch_register(backend::Register{
                backend::FloatRegister::Fa0})

            );
            builder.append_instruction(fsgnjs_instruction);
          } else {
            auto addi_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::ADDI, asm_dst_id,
              builder.fetch_register(backend::Register{
                backend::GeneralRegister::A0}),
              builder.fetch_immediate(0)
            );
            builder.append_instruction(addi_instruction);
          }
        }
      },
      [&](ir::instruction::GetElementPtr& gep) {
        auto asm_dst_id = codegen_operand(
          gep.dst_id, ir_context, builder, codegen_context, false, false
        );
        auto asm_ptr_id = codegen_operand(
          gep.ptr_id, ir_context, builder, codegen_context, false, false
        );

        auto asm_ptr = builder.context.get_operand(asm_ptr_id);

        if (asm_ptr->is_global()) {
          auto asm_tmp_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );
          // Pseudo la
          auto la_instruction = builder.fetch_pseudo_load_instruction(
            backend::instruction::PseudoLoad::LA, asm_tmp_id, asm_ptr_id
          );
          builder.append_instruction(la_instruction);

          asm_ptr_id = asm_tmp_id;
        }

        auto ir_basis_type = gep.basis_type;

        for (auto ir_operand_id : gep.index_id_list) {
          auto asm_operand_id = codegen_operand(
            ir_operand_id, ir_context, builder, codegen_context, false, false
          );
          auto size = ir::get_size(ir_basis_type);

          auto asm_size_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );
          auto li_instruction = builder.fetch_li_instruction(
            asm_size_id, builder.fetch_immediate((int32_t)(size / 8))
          );
          builder.append_instruction(li_instruction);

          auto asm_mul_dst_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );
          auto mul_instruction = builder.fetch_binary_instruction(
            backend::instruction::Binary::Op::MUL, asm_mul_dst_id,
            asm_operand_id, asm_size_id
          );

          auto asm_add_dst_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );
          auto add_instruction = builder.fetch_binary_instruction(
            backend::instruction::Binary::Op::ADD, asm_add_dst_id, asm_ptr_id,
            asm_mul_dst_id
          );

          builder.append_instruction(mul_instruction);
          builder.append_instruction(add_instruction);

          asm_ptr_id = asm_add_dst_id;

          if (ir_basis_type->as<ir::type::Array>().has_value()) {
            ir_basis_type =
              ir_basis_type->as<ir::type::Array>().value().element_type;
          } else {
            break;
          }
        }

        // mv
        auto addi_instruction = builder.fetch_binary_imm_instruction(
          backend::instruction::BinaryImm::Op::ADDI, asm_dst_id, asm_ptr_id,
          builder.fetch_immediate(0)
        );
        builder.append_instruction(addi_instruction);
      },
      [&](ir::instruction::Ret& ret) {
        auto ir_maybe_value_id = ret.maybe_value_id;
        if (ir_maybe_value_id.has_value()) {
          auto asm_value_id = codegen_operand(
            ir_maybe_value_id.value(), ir_context, builder, codegen_context,
            false, false
          );

          auto asm_value = builder.context.get_operand(asm_value_id);

          if (asm_value->is_float()) {
            auto fa0_id = builder.fetch_register(backend::Register{
              backend::FloatRegister::Fa0});

            // fmv.s
            auto fsgnjs_instruction = builder.fetch_float_binary_instruction(
              backend::instruction::FloatBinary::FSGNJ,
              backend::instruction::FloatBinary::S, fa0_id, asm_value_id,
              asm_value_id
            );

            builder.append_instruction(fsgnjs_instruction);

          } else {
            auto a0_id = builder.fetch_register(backend::Register{
              backend::GeneralRegister::A0});

            auto mv_instruction = builder.fetch_binary_imm_instruction(
              backend::instruction::BinaryImm::Op::ADDI, a0_id, asm_value_id,
              builder.fetch_immediate(0)
            );

            builder.append_instruction(mv_instruction);
          }
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
  bool use_fmv,
  bool force_keep_imm,
  bool keep_local_memory
) {
  auto ir_operand = ir_context.get_operand(ir_operand_id);
  auto& ir_operand_kind = ir_operand->kind;
  auto ir_operand_type = ir_operand->type;

  bool is_float = ir_operand_type->as<ir::type::Float>().has_value();

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

          if (asm_temp->is_local_memory() && !keep_local_memory) {
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

          codegen_context.operand_map[ir_operand_id] = asm_operand_id;
        } else {
          asm_operand_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );
          codegen_context.operand_map[ir_operand_id] = asm_operand_id;
        }
      },
      [&](ir::operand::ConstantPtr ir_constant) {
        if (is_float) {
          float value = std::get<float>(ir_constant->kind);
          uint32_t bits = *reinterpret_cast<uint32_t*>(&value);
          auto asm_imm_id = builder.fetch_immediate((uint32_t)bits);

          auto asm_temp_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );
          if (force_keep_imm) {
            asm_operand_id = asm_imm_id;
          } else if (check_utype_immediate(bits)) {
            auto lui_instruction = builder.fetch_lui_instruction(
              asm_temp_id, builder.fetch_immediate((uint32_t)(bits >> 12))
            );
            builder.append_instruction(lui_instruction);
          } else {
            auto li_instruction =
              builder.fetch_li_instruction(asm_temp_id, asm_imm_id);
            builder.append_instruction(li_instruction);
          }

          if (force_keep_imm) {
            // Not using fmv instruction.
          } else if (use_fmv) {
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
          // TODO: Support 64-bit immediate according to ir type.
          int value = std::get<int>(ir_constant->kind);
          if (force_keep_imm) {
            asm_operand_id = builder.fetch_immediate((int32_t)value);
          } else if (check_itype_immediate(value) && try_keep_imm) {
            auto asm_imm_id = builder.fetch_immediate((int32_t)value);
            asm_operand_id = asm_imm_id;
          } else {
            asm_operand_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );
            uint32_t bits = *reinterpret_cast<uint32_t*>(&value);
            if (check_utype_immediate(bits)) {
              auto lui_instruction = builder.fetch_lui_instruction(
                asm_operand_id, builder.fetch_immediate((uint32_t)(bits >> 12))
              );
              builder.append_instruction(lui_instruction);
            } else {
              auto asm_imm_id = builder.fetch_immediate((int32_t)value);
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
      [&](ir::operand::Parameter& ir_parameter) {
        if (!codegen_context.operand_map.count(ir_operand_id)) {
          throw std::runtime_error("parameter is not initialized.");
        }

        auto ir_operand = ir_context.get_operand(ir_operand_id);

        bool is_float = ir_operand->type->as<ir::type::Float>().has_value();

        auto asm_param_id = codegen_context.operand_map.at(ir_operand_id);
        auto asm_param = builder.context.get_operand(asm_param_id);

        if (asm_param->is_reg() || asm_param->is_vreg()) {
          asm_operand_id = asm_param_id;
        } else if (asm_param->is_local_memory()) {
          int offset = std::get<backend::LocalMemory>(asm_param->kind).offset;

          // parameters are index by fp
          auto asm_fp_id = builder.fetch_register(backend::Register{
            backend::GeneralRegister::S0});

          if (is_float) {
            if (check_itype_immediate(offset)) {
              asm_operand_id = builder.fetch_virtual_register(
                backend::VirtualRegisterKind::Float
              );
              auto fld_instruction = builder.fetch_float_load_instruction(
                backend::instruction::FloatLoad::Op::FLD, asm_operand_id,
                asm_fp_id, builder.fetch_immediate(offset)
              );
              builder.append_instruction(fld_instruction);
            } else {
              asm_operand_id = builder.fetch_virtual_register(
                backend::VirtualRegisterKind::Float
              );
              auto asm_tmp_id = builder.fetch_register(backend::Register{
                backend::GeneralRegister::T2});
              auto li_instruction = builder.fetch_li_instruction(
                asm_tmp_id, builder.fetch_immediate(offset)
              );
              builder.append_instruction(li_instruction);
              auto add_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::ADD, asm_tmp_id, asm_fp_id,
                asm_tmp_id
              );
              builder.append_instruction(add_instruction);
              auto fld_instruction = builder.fetch_float_load_instruction(
                backend::instruction::FloatLoad::Op::FLD, asm_operand_id,
                asm_tmp_id, builder.fetch_immediate(0)
              );
              builder.append_instruction(fld_instruction);
            }

          } else {
            if (check_itype_immediate(offset)) {
              asm_operand_id = builder.fetch_virtual_register(
                backend::VirtualRegisterKind::General
              );
              auto ld_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LD, asm_operand_id, asm_fp_id,
                builder.fetch_immediate(offset)
              );
              builder.append_instruction(ld_instruction);
            } else {
              asm_operand_id = builder.fetch_virtual_register(
                backend::VirtualRegisterKind::General
              );
              auto asm_tmp_id = builder.fetch_register(backend::Register{
                backend::GeneralRegister::T2});
              auto li_instruction = builder.fetch_li_instruction(
                asm_tmp_id, builder.fetch_immediate(offset)
              );
              builder.append_instruction(li_instruction);
              auto add_instruction = builder.fetch_binary_instruction(
                backend::instruction::Binary::Op::ADD, asm_tmp_id, asm_fp_id,
                asm_tmp_id
              );
              builder.append_instruction(add_instruction);
              auto ld_instruction = builder.fetch_load_instruction(
                backend::instruction::Load::Op::LD, asm_operand_id, asm_tmp_id,
                builder.fetch_immediate(0)
              );
              builder.append_instruction(ld_instruction);
            }
          }
        } else {
          throw std::runtime_error("Invalid parameter kind.");
        }
      },
      [](auto& k) { throw std::runtime_error("Invalid operand kind."); },
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
    backend::greedy_allocation(function, builder);
  }
}

}  // namespace syc
