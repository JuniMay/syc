#include "frontend/irgen.h"

#include "frontend/ast.h"
#include "frontend/symtable.h"
#include "ir/operand.h"

namespace syc {

void irgen(const AstCompunit& compunit, IrBuilder& builder) {
  for (auto stmt : compunit.stmts) {
    irgen_stmt(stmt, compunit.symtable, builder);
  }
}

void irgen_stmt(
  AstStmtPtr stmt,
  AstSymbolTablePtr symtable,
  IrBuilder& builder
) {
  std::visit(
    overloaded{
      [symtable, &builder](const frontend::ast::stmt::Decl& kind) {
        switch (kind.scope) {
          case frontend::Scope::Global: {
            for (auto [ast_type, name, maybe_ast_expr] : kind.defs) {
              auto ir_type = irgen_type(ast_type, builder).value();
              auto is_constant_value = kind.is_const;
              IrOperandID ir_operand_id = 0;
              if (maybe_ast_expr.has_value()) {
                auto ast_expr = maybe_ast_expr.value();
                if (!ast_expr->is_comptime()) {
                  std::cerr
                    << "Error: initial value of global variable/constant must "
                       "be a constant expression."
                    << std::endl;
                  abort();
                }
                auto ir_constant = irgen_comptime_value(
                  ast_expr->get_comptime_value().value(), builder
                );
                auto ir_init_operand_id =
                  builder.fetch_operand(ir_type, ir_constant);
                ir_operand_id = builder.fetch_global_operand(
                  ir_type, name, is_constant_value, ir_init_operand_id
                );
              } else {
                auto ir_constant = irgen_comptime_value(
                  frontend::create_zero_comptime_value(ast_type), builder
                );
                auto ir_init_operand_id =
                  builder.fetch_operand(ir_type, ir_constant);
                ir_operand_id = builder.fetch_global_operand(
                  ir_type, name, is_constant_value, ir_init_operand_id
                );
              }
              auto maybe_symbol = symtable->lookup(name);
              if (maybe_symbol.has_value()) {
                auto symbol = maybe_symbol.value();
                symbol->set_ir_operand_id(ir_operand_id);
              } else {
                std::string error_message =
                  "Error: global symbol `" + name +
                  "` is not found when generating IR.";
                throw std::runtime_error(error_message);
              }
            }
          }
          case frontend::Scope::Local: {
            // TODO
          }
          default: {
            // TODO
          }
        }
      },
      [&builder](const auto&) {

      },
    },
    stmt->kind
  );
}

void irgen_expr(
  AstExprPtr expr,
  AstSymbolTablePtr symtable,
  IrBuilder& builder
) {}

IrConstantPtr
irgen_comptime_value(AstComptimeValuePtr value, IrBuilder& builder) {
  auto ir_type = irgen_type(value->type, builder).value();
  auto ir_constant = std::visit(
    overloaded{
      [ir_type, &builder](int v) { return builder.fetch_constant(ir_type, v); },
      [ir_type, &builder](float v) {
        return builder.fetch_constant(ir_type, v);
      },
      [ir_type, &builder](bool v) {
        if (v) {
          return builder.fetch_constant(ir_type, (int)1);
        } else {
          return builder.fetch_constant(ir_type, (int)0);
        }
      },
      [ir_type, &builder](const std::vector<AstComptimeValuePtr>& v) {
        std::vector<IrConstantPtr> ir_constant_list;
        for (auto comptime_value : v) {
          ir_constant_list.push_back(
            irgen_comptime_value(comptime_value, builder)
          );
        }
        return builder.fetch_constant(ir_type, ir_constant_list);
      },
      [ir_type, &builder](frontend::Zeroinitializer v) {
        return builder.fetch_constant(ir_type, ir::operand::Zeroinitializer{});
      }},
    value->kind
  );
  return ir_constant;
}

std::optional<IrTypePtr> irgen_type(AstTypePtr type, IrBuilder& builder) {
  if (type->is_int()) {
    return builder.fetch_i32_type();
  } else if (type->is_bool()) {
    return builder.fetch_i1_type();
  } else if (type->is_float()) {
    return builder.fetch_float_type();
  } else if (type->is_void()) {
    return builder.fetch_void_type();
  } else if (type->is_pointer()) {
    return builder.fetch_pointer_type(
      irgen_type(type->get_value_type().value(), builder).value()
    );
  } else if (type->is_array()) {
    auto element_type = type->get_element_type().value();
    auto maybe_length =
      std::get<frontend::type::Array>(type->kind).maybe_length;
    if (maybe_length.has_value()) {
      return builder.fetch_array_type(
        maybe_length.value(), irgen_type(element_type, builder).value()
      );
    } else {
      return builder.fetch_pointer_type(
        irgen_type(element_type, builder).value()
      );
    }
  } else {
    return std::nullopt;
  }
}

}  // namespace syc