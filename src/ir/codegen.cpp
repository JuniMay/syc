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

  asm_register_allocation(builder.context);

  for (auto& [ir_function_name, ir_function] : ir_context.function_table) {
    if (ir_function->is_declare) {
      continue;
    }
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

  // TODO: saved registers

  auto asm_function = builder.context.get_function(function_name);
  auto entry_block = asm_function->head_basic_block->next;

  auto stack_frame_size = asm_function->stack_frame_size;

  // store ra
  auto ra_id = builder.fetch_register(Register{GeneralRegister::Ra});

  auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

  auto sd_instruction = builder.fetch_store_instruction(
    instruction::Store::Op::SD, sp_id, ra_id,
    builder.fetch_immediate((int32_t)stack_frame_size)
  );

  asm_function->stack_frame_size += 8;
  stack_frame_size += 8;

  auto addi_instruction = builder.fetch_binary_imm_instruction(
    instruction::BinaryImm::Op::ADDI, sp_id, sp_id,
    builder.fetch_immediate(-(int32_t)stack_frame_size)
  );

  entry_block->prepend_instruction(sd_instruction);
  entry_block->prepend_instruction(addi_instruction);
}

void codegen_function_epilouge(
  std::string function_name,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  using namespace backend;

  // TODO: saved registers

  auto asm_function = builder.context.get_function(function_name);
  auto exit_block = asm_function->tail_basic_block->prev.lock();

  auto stack_frame_size = asm_function->stack_frame_size;

  // restore ra
  auto ra_id = builder.fetch_register(Register{GeneralRegister::Ra});

  auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

  auto ld_instruction = builder.fetch_load_instruction(
    instruction::Load::Op::LD, ra_id, sp_id,
    builder.fetch_immediate((int32_t)stack_frame_size - 8)
  );

  auto addi_instruction = builder.fetch_binary_imm_instruction(
    instruction::BinaryImm::Op::ADDI, sp_id, sp_id,
    builder.fetch_immediate((int32_t)stack_frame_size)
  );

  exit_block->prepend_instruction(addi_instruction);
  exit_block->prepend_instruction(ld_instruction);
}

void codegen_basic_block(
  IrBasicBlockPtr ir_basic_block,
  IrContext& ir_context,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  auto basic_block = builder.fetch_basic_block();
  builder.set_curr_basic_block(basic_block);

  auto curr_ir_instruction = ir_basic_block->head_instruction->next;
  while (curr_ir_instruction != ir_basic_block->tail_instruction) {
    codegen_instruction(
      curr_ir_instruction, ir_context, builder, codegen_context
    );
    curr_ir_instruction = curr_ir_instruction->next;
  }

  builder.curr_function->append_basic_block(basic_block);
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

        auto ir_value = ir_context.get_operand(ir_value_id);

        AsmOperandID asm_value_id;

        bool is_float =
          std::holds_alternative<ir::type::Float>(*ir_value->type);

        bool is_imm = false;

        if (auto ir_constant = std::get_if<ir::operand::ConstantPtr>(&ir_value->kind)) {
          is_imm = true;
          auto ir_constant_kind = (*ir_constant)->kind;
          auto ir_constant_type = (*ir_constant)->type;

          if (is_float) {
            float ir_constant_value = std::get<float>(ir_constant_kind);
            uint32_t imm = *reinterpret_cast<uint32_t*>(&ir_constant_value);

            auto vreg_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );
            auto li_instruction = builder.fetch_li_instruction(
              vreg_id, builder.fetch_immediate(imm)
            );
            builder.append_instruction(li_instruction);

            asm_value_id = vreg_id;
          } else {
            int ir_constant_value = std::get<int>(ir_constant_kind);
            uint32_t imm = *reinterpret_cast<uint32_t*>(&ir_constant_value);

            auto vreg_id = builder.fetch_virtual_register(
              backend::VirtualRegisterKind::General
            );
            auto li_instruction = builder.fetch_li_instruction(
              vreg_id, builder.fetch_immediate(imm)
            );
            builder.append_instruction(li_instruction);

            asm_value_id = vreg_id;
          }
        } else {
          auto asm_value_id = codegen_context.get_asm_operand_id(ir_value_id);
        }
        auto asm_ptr_id = codegen_context.get_asm_operand_id(ir_ptr_id);

        auto asm_value = builder.context.get_operand(asm_value_id);
        auto asm_ptr = builder.context.get_operand(asm_ptr_id);

        if (auto asm_local_memory = std::get_if<backend::LocalMemory>(&asm_ptr->kind)) {
          auto offset = asm_local_memory->offset;
          auto sp_id = builder.fetch_register(backend::Register{
            backend::GeneralRegister::Sp});

          if (is_float && !is_imm) {
            auto op = backend::instruction::FloatStore::Op::FSW;
            auto instruction = builder.fetch_float_store_instruction(
              op, sp_id, asm_value_id, builder.fetch_immediate((int32_t)offset)
            );

            builder.append_instruction(instruction);
          } else {
            auto op = backend::instruction::Store::SW;
            auto instruction = builder.fetch_store_instruction(
              op, sp_id, asm_value_id, builder.fetch_immediate((int32_t)offset)
            );

            builder.append_instruction(instruction);
          }

        } else if (auto asm_global = std::get_if<backend::Global>(&asm_ptr->kind)) {
          // lui %hi of the address
          auto asm_global_hi_id =
            builder.fetch_operand(*asm_global, backend::Modifier::Hi);

          auto vreg_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );

          auto lui_instruction =
            builder.fetch_lui_instruction(vreg_id, asm_global_hi_id);

          builder.append_instruction(lui_instruction);

          // sw %lo of the address
          auto asm_global_lo_id =
            builder.fetch_operand(*asm_global, backend::Modifier::Lo);

          if (is_float && !is_imm) {
            auto fsw_instruction = builder.fetch_float_store_instruction(
              backend::instruction::FloatStore::Op::FSW, vreg_id, asm_value_id,
              asm_global_lo_id
            );

            builder.append_instruction(fsw_instruction);
          } else {
            auto sw_instruction = builder.fetch_store_instruction(
              backend::instruction::Store::SW, vreg_id, asm_value_id,
              asm_global_lo_id
            );

            builder.append_instruction(sw_instruction);
          }

        } else if (auto asm_register = std::get_if<backend::Register>(&asm_ptr->kind)) {
          if (is_float && !is_imm) {
            auto fsw_instruction = builder.fetch_float_store_instruction(
              backend::instruction::FloatStore::Op::FSW, asm_ptr_id,
              asm_value_id, builder.fetch_immediate(0)
            );

            builder.append_instruction(fsw_instruction);
          } else {
            auto sw_instruction = builder.fetch_store_instruction(
              backend::instruction::Store::SW, asm_ptr_id, asm_value_id,
              builder.fetch_immediate(0)
            );

            builder.append_instruction(sw_instruction);
          }
        } else {
          throw std::runtime_error("Invalid operand for store instruction.");
        }
      },
      [&](ir::instruction::Load& ir_load) {
        auto ir_dst_id = ir_load.dst_id;
        auto ir_ptr_id = ir_load.ptr_id;

        auto ir_dst = ir_context.get_operand(ir_dst_id);
        auto asm_dst_id =
          codegen_operand(ir_dst_id, ir_context, builder, codegen_context);
        auto asm_ptr_id = codegen_context.get_asm_operand_id(ir_ptr_id);

        auto asm_ptr = builder.context.get_operand(asm_ptr_id);

        bool is_float = false;

        if (std::holds_alternative<ir::type::Float>(*ir_dst->type)) {
          is_float = true;
        }

        if (auto asm_global = std::get_if<backend::Global>(&asm_ptr->kind)) {
          auto asm_global_hi_id =
            builder.fetch_operand(*asm_global, backend::Modifier::Hi);

          auto vreg_id =
            builder.fetch_virtual_register(backend::VirtualRegisterKind::General
            );

          auto lui_instruction =
            builder.fetch_lui_instruction(vreg_id, asm_global_hi_id);

          builder.append_instruction(lui_instruction);

          auto asm_global_lo_id =
            builder.fetch_operand(*asm_global, backend::Modifier::Lo);

          if (is_float) {
            auto flw_instruction = builder.fetch_float_load_instruction(
              backend::instruction::FloatLoad::Op::FLW, asm_dst_id, vreg_id,
              asm_global_lo_id
            );
            builder.append_instruction(flw_instruction);
          } else {
            auto lw_instruction = builder.fetch_load_instruction(
              backend::instruction::Load::LW, asm_dst_id, vreg_id,
              asm_global_lo_id
            );
            builder.append_instruction(lw_instruction);
          }

          codegen_context.operand_map[ir_dst_id] = asm_dst_id;
        } else if (auto asm_local_memory = std::get_if<backend::LocalMemory>(&asm_ptr->kind)) {
          auto offset = asm_local_memory->offset;
          auto sp_id = builder.fetch_register(backend::Register{
            backend::GeneralRegister::Sp});

          if (is_float) {
            auto flw_instruction = builder.fetch_float_load_instruction(
              backend::instruction::FloatLoad::Op::FLW, asm_dst_id, sp_id,
              builder.fetch_immediate((int32_t)offset)
            );
            builder.append_instruction(flw_instruction);
          } else {
            auto lw_instruction = builder.fetch_load_instruction(
              backend::instruction::Load::Op::LW, asm_dst_id, sp_id,
              builder.fetch_immediate((int32_t)offset)
            );
            builder.append_instruction(lw_instruction);
          }
        } else if (auto asm_register = std::get_if<backend::Register>(&asm_ptr->kind)) {
          if (is_float) {
            auto flw_instruction = builder.fetch_float_load_instruction(
              backend::instruction::FloatLoad::Op::FLW, asm_dst_id, asm_ptr_id,
              builder.fetch_immediate((int32_t)0)
            );
            builder.append_instruction(flw_instruction);
          } else {
            auto lw_instruction = builder.fetch_load_instruction(
              backend::instruction::Load::Op::LW, asm_dst_id, asm_ptr_id,
              builder.fetch_immediate((int32_t)0)
            );
            builder.append_instruction(lw_instruction);
          }
        }
      },
      [&](ir::instruction::Binary& ir_binary) {

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
  CodegenContext& codegen_context
) {
  if (codegen_context.operand_map.find(ir_operand_id) !=
      codegen_context.operand_map.end()) {
    return codegen_context.operand_map[ir_operand_id];
  }

  auto ir_operand = ir_context.get_operand(ir_operand_id);
  auto& ir_operand_kind = ir_operand->kind;
  auto ir_operand_type = ir_operand->type;

  AsmOperandID asm_operand_id;

  if (std::holds_alternative<ir::operand::ConstantPtr>(ir_operand_kind)) {
    auto& ir_constant_kind =
      std::get<ir::operand::ConstantPtr>(ir_operand_kind)->kind;
    auto ir_constant_type =
      std::get<ir::operand::ConstantPtr>(ir_operand_kind)->type;

    if (int ir_value = *std::get_if<int>(&ir_constant_kind)) {
      asm_operand_id = builder.fetch_immediate((int32_t)ir_value);
    } else if (float ir_value = *std::get_if<float>(&ir_constant_kind)) {
      asm_operand_id =
        builder.fetch_immediate(*reinterpret_cast<uint32_t*>(&ir_value));
    } else {
      throw std::runtime_error(
        "Invalid type for global variable initialization."
      );
    }
  } else if (std::holds_alternative<ir::operand::Arbitrary>(ir_operand_kind)) {
    if (std::holds_alternative<ir::type::Float>(*ir_operand_type)) {
      asm_operand_id =
        builder.fetch_virtual_register(backend::VirtualRegisterKind::Float);
    } else {
      asm_operand_id =
        builder.fetch_virtual_register(backend::VirtualRegisterKind::General);
    }

    codegen_context.operand_map[ir_operand_id] = asm_operand_id;
  } else {
    throw std::runtime_error("Invalid operand kind.");
  }

  return asm_operand_id;
}

/// Perform register allocation.
void asm_register_allocation(AsmContext& context) {
  // TODO
}

/// Perform instruction scheduling.
void asm_instruction_scheduling(AsmContext& context) {
  // TODO
}

}  // namespace syc
