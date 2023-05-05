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
            break;
          }
          case frontend::Scope::Local: {
            for (auto [ast_type, name, maybe_ast_expr] : kind.defs) {
              auto ir_type = irgen_type(ast_type, builder).value();

              auto ir_dst_operand_id = builder.fetch_arbitrary_operand(
                builder.fetch_pointer_type(ir_type)
              );

              auto alloca_instruction = builder.fetch_alloca_instruction(
                ir_dst_operand_id, ir_type, std::nullopt, std::nullopt,
                std::nullopt
              );

              // alloca instructions are prepended to the current function.
              builder.prepend_instruction_to_curr_function(alloca_instruction);

              auto maybe_symbol = symtable->lookup(name);
              if (maybe_symbol.has_value()) {
                auto symbol = maybe_symbol.value();
                symbol->set_ir_operand_id(ir_dst_operand_id);
              } else {
                std::string error_message =
                  "Error: local symbol `" + name +
                  "` is not found when generating IR.";
                throw std::runtime_error(error_message);
              }

              if (maybe_ast_expr.has_value()) {
                auto ast_expr = maybe_ast_expr.value();
                auto ir_init_operand_id =
                  irgen_expr(ast_expr, symtable, builder);
                auto store_instruction = builder.fetch_store_instruction(
                  ir_init_operand_id, ir_dst_operand_id, std::nullopt
                );
                builder.append_instruction(store_instruction);
              }

              break;
            }
          }
          default: {
            // TODO

            break;
          }
        }
      },
      [&builder](const frontend::ast::stmt::FuncDef& kind) {
        auto symbol = kind.symbol_entry;

        std::string function_name = symbol->name;
        std::vector<IrOperandID> parameter_id_list;

        auto ast_func_type = symbol->type;

        if (!ast_func_type->is_function()) {
          std::string error_message =
            "Error: symbol `" + symbol->name + "` is not a  function.";
          throw std::runtime_error(error_message);
        }

        auto ast_func_ret_type = ast_func_type->get_ret_type().value();
        auto ir_func_ret_type = irgen_type(ast_func_ret_type, builder).value();

        for (auto param_name : kind.param_names) {
          auto maybe_ast_param_symbol = kind.symtable->lookup(param_name);

          if (!maybe_ast_param_symbol.has_value()) {
            std::string error_messgae =
              "Error: parameter `" + param_name + "` is not defined.";
            throw std::runtime_error(error_messgae);
          }

          auto ast_param_symbol = maybe_ast_param_symbol.value();
          auto ast_param_type = ast_param_symbol->type;
          auto ir_param_type = irgen_type(ast_param_type, builder).value();
          auto ir_param_operand_id =
            builder.fetch_parameter_operand(ir_param_type, param_name);
          ast_param_symbol->set_ir_operand_id(ir_param_operand_id);
          parameter_id_list.push_back(ir_param_operand_id);
        }

        bool is_declare = !kind.maybe_body.has_value();

        builder.add_function(
          function_name, parameter_id_list, ir_func_ret_type, is_declare
        );

        if (!is_declare) {
          irgen_stmt(kind.maybe_body.value(), kind.symtable, builder);
        }
      },
      [&builder](const frontend::ast::stmt::Block& kind) {
        auto ir_basic_block = builder.fetch_basic_block();
        builder.append_basic_block(ir_basic_block);
        builder.set_curr_basic_block(ir_basic_block);

        for (auto stmt : kind.stmts) {
          irgen_stmt(stmt, kind.symtable, builder);
        }
      },
      [&builder](const auto&) {
        // TODO
      },
    },
    stmt->kind
  );
}

IrOperandID
irgen_expr(AstExprPtr expr, AstSymbolTablePtr symtable, IrBuilder& builder) {
  return std::visit(
    overloaded{
      [&builder](const frontend::ast::expr::Constant& kind) {
        auto ir_constant = irgen_comptime_value(kind.value, builder);
        auto ir_constant_operand_id =
          builder.fetch_operand(ir_constant->type, ir_constant);
        return ir_constant_operand_id;
      },
      [symtable,
       &builder](const frontend::ast::expr::Binary& kind) -> IrOperandID {
        auto lhs_operand_id = irgen_expr(kind.lhs, symtable, builder);
        auto rhs_operand_id = irgen_expr(kind.rhs, symtable, builder);

        auto symbol = kind.symbol;

        auto ir_dst_operand_id = builder.fetch_arbitrary_operand(
          irgen_type(symbol->type, builder).value()
        );

        symbol->set_ir_operand_id(ir_dst_operand_id);

        // TODO
        switch (kind.op) {
          case AstBinaryOp::Add:
            break;
          case AstBinaryOp::Sub:
            break;
          case AstBinaryOp::Div:
            break;
          case AstBinaryOp::Mul:
            break;
          case AstBinaryOp::Mod:
            break;
          case AstBinaryOp::Lt:
            break;
          case AstBinaryOp::Le:
            break;
          case AstBinaryOp::Gt:
            break;
          case AstBinaryOp::Ge:
            break;
          case AstBinaryOp::Eq:
            break;
          case AstBinaryOp::Ne:
            break;
          case AstBinaryOp::LogicalAnd:
            break;
          case AstBinaryOp::LogicalOr:
            break;
          case AstBinaryOp::Index:
            break;
          default:
            // unreachable.
            break;
        }
        return 0;
      },
      [](const auto& kind) -> IrOperandID { return 0; },
    },
    expr->kind
  );
}

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
    std::string error_message = "Erro: cannot generate a function type in ir.";
    std::cerr << error_message << std::endl;
    return std::nullopt;
  }
}

}  // namespace syc