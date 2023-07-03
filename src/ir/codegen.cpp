#include "ir/codegen.h"
#include "backend/builder.h"
#include "ir/operand.h"
#include "ir/type.h"

namespace syc {

void codegen(
  ir::Context& ir_context,
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
    codegen_function(ir_function, builder, codegen_context);
  }
}

void codegen_function(
  IrFunctionPtr ir_function,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  // TODO
}

void codegen_basic_block(
  IrBasicBlockPtr ir_basic_block,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  // TODO
}

void codegen_instruction(
  IrInstructionPtr ir_instruction,
  AsmBuilder& builder,
  CodegenContext& codegen_context
) {
  // TODO
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
