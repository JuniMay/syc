#include "frontend/irgen.h"
#include "frontend/symtable.h"

namespace syc {

void irgen_compunit(
  frontend::ast::Compunit& ast_compunit,
  ir::Builder& ir_builder
) {
  for (auto& ast_stmt : ast_compunit.stmts) {
    irgen_stmt(ast_stmt, ir_builder);
  }
}

void irgen_expr(frontend::ast::ExprPtr ast_expr, ir::Builder& ir_builder) {}

void irgen_stmt(frontend::ast::StmtPtr ast_stmt, ir::Builder& ir_builder) {
  std::visit(
    overloaded{
      [&](frontend::ast::stmt::Blank& kind) {},
      [&](frontend::ast::stmt::Block& kind) {
        auto ir_block = ir_builder.fetch_basic_block();

        ir_builder.append_basic_block(ir_block);
        ir_builder.set_curr_basic_block(ir_block);

        for (auto& ast_stmt : kind.stmts) {
          irgen_stmt(ast_stmt, ir_builder);
        }
      },
      [&](frontend::ast::stmt::FuncDef& kind) {
        auto function_name = kind.symbol_entry->name;
        std::vector<ir::OperandID> parameter_id_list = {};

        for (const auto& param_name : kind.param_names) {
          auto param_symbol_entry =
            kind.symtable->lookup(param_name, false).value();
          auto ir_operand_id =
            irgen_symbol_entry(param_symbol_entry, ir_builder);
          parameter_id_list.push_back(ir_operand_id);
        }

        auto ir_return_type = irgen_type(
          kind.symbol_entry->type->get_ret_type().value(), ir_builder
        );

        if (kind.maybe_body.has_value()) {
          ir_builder.add_function(
            function_name, parameter_id_list, ir_return_type, false
          );
          irgen_stmt(kind.maybe_body.value(), ir_builder);
        } else {
          ir_builder.add_function(
            function_name, parameter_id_list, ir_return_type, true
          );
        }
      },
      [&](auto& kind) {},
    },
    ast_stmt->kind
  );
}

ir::TypePtr irgen_type(frontend::TypePtr ast_type, ir::Builder& ir_builder) {
  return std::visit(
    overloaded{
      [&](const frontend::type::Integer& kind) -> ir::TypePtr {
        switch (kind.size) {
          case 32:
            return ir_builder.fetch_i32_type();
          case 1:
            return ir_builder.fetch_i1_type();
          default:
            throw std::runtime_error("Unknown integer size");
        }
        return nullptr;
      },
      [&](const frontend::type::Float&) {
        return ir_builder.fetch_float_type();
      },
      [&](const frontend::type::Void&) { return ir_builder.fetch_void_type(); },
      [&](const frontend::type::Array& kind) {
        if (!kind.maybe_size.has_value()) {
          return ir_builder.fetch_pointer_type(
            irgen_type(kind.element_type, ir_builder)
          );
        }
        return ir_builder.fetch_array_type(
          kind.maybe_size, irgen_type(kind.element_type, ir_builder)
        );
      },
      [&](const frontend::type::Pointer& kind) {
        return ir_builder.fetch_pointer_type(
          irgen_type(kind.value_type, ir_builder)
        );
      },
      [&](const auto& kind) -> ir::TypePtr { return nullptr; }},
    ast_type->kind
  );
}

ir::OperandID irgen_symbol_entry(
  frontend::SymbolEntryPtr ast_symbol_entry,
  ir::Builder& ir_builder
) {
  if (ast_symbol_entry->has_ir_operand()) {
    return ast_symbol_entry->maybe_ir_operand_id.value();
  }

  ir::OperandID ir_operand_id;
  switch (ast_symbol_entry->scope) {
    case frontend::Scope::Global: {
      // TODO: decide zeroinitializer.
      if (ast_symbol_entry->maybe_value.has_value() && !ast_symbol_entry->type->is_array()) {
        ir_operand_id = ir_builder.fetch_global_operand(
          irgen_type(ast_symbol_entry->type, ir_builder),
          ast_symbol_entry->name, ast_symbol_entry->is_const, false,
          {
            irgen_comtime_value(
              ast_symbol_entry->maybe_value.value(), ir_builder
            ),
          }
        );
      } else {
        ir_operand_id = ir_builder.fetch_global_operand(
          irgen_type(ast_symbol_entry->type, ir_builder),
          ast_symbol_entry->name, ast_symbol_entry->is_const, false, {}
        );
      }
      break;
    }
    case frontend::Scope::Temp:
    case frontend::Scope::Local: {
      ir_operand_id = ir_builder.fetch_arbitrary_operand(
        irgen_type(ast_symbol_entry->type, ir_builder)
      );
      break;
    }
    case frontend::Scope::Param: {
      ir_operand_id = ir_builder.fetch_parameter_operand(
        irgen_type(ast_symbol_entry->type, ir_builder), ast_symbol_entry->name
      );
      break;
    }
  }
  ast_symbol_entry->maybe_ir_operand_id = std::make_optional(ir_operand_id);
  return ir_operand_id;
}

ir::OperandID irgen_comtime_value(
  frontend::ComptimeValue ast_comptime_value,
  ir::Builder& ir_builder
) {
  auto ir_type = irgen_type(ast_comptime_value.type, ir_builder);
  return std::visit(
    overloaded{
      [&](bool value) {
        return ir_builder.fetch_immediate_operand(ir_type, (int)value);
      },
      [&](int value) {
        return ir_builder.fetch_immediate_operand(ir_type, value);
      },
      [&](float value) {
        return ir_builder.fetch_immediate_operand(ir_type, value);
      },
    },
    ast_comptime_value.value
  );
}

}  // namespace syc