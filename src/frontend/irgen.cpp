#include "frontend/irgen.h"

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
            }
            break;
          }
          default: {
            std::string error_message =
              "Error: scope is not supported for declaration";
            throw std::runtime_error(error_message);
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

        // set the operand in the symbol to the parameter first.
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

        // if the function is just a declaration.
        bool is_declare = !kind.maybe_body.has_value();

        builder.add_function(
          function_name, parameter_id_list, ir_func_ret_type, is_declare
        );

        if (!is_declare) {
          auto ast_body = kind.maybe_body.value();

          const auto& ast_block =
            std::get<frontend::ast::stmt::Block>(ast_body->kind);

          auto ir_func_entry_block = builder.fetch_basic_block();
          builder.append_basic_block(ir_func_entry_block);
          builder.set_curr_basic_block(ir_func_entry_block);

          // store all the argument/parameter to the memory and set the operand
          // id of the param symbol to the corresponding pointer.
          for (auto param_name : kind.param_names) {
            auto maybe_ast_param_symbol = kind.symtable->lookup(param_name);

            if (!maybe_ast_param_symbol.has_value()) {
              std::string error_messgae =
                "Error: parameter `" + param_name + "` is not defined.";
              throw std::runtime_error(error_messgae);
            }

            auto ast_param_symbol = maybe_ast_param_symbol.value();
            auto ir_param_operand_id =
              ast_param_symbol->maybe_ir_operand_id.value();
            auto ir_param_operand =
              builder.context.get_operand(ir_param_operand_id);

            auto ir_alloca_dst_id = builder.fetch_arbitrary_operand(
              builder.fetch_pointer_type(ir_param_operand->type)
            );

            auto alloca_instruction = builder.fetch_alloca_instruction(
              ir_alloca_dst_id, ir_param_operand->type, std::nullopt,
              std::nullopt, std::nullopt
            );

            builder.prepend_instruction_to_curr_function(alloca_instruction);

            auto store_instruction = builder.fetch_store_instruction(
              ir_param_operand_id, ir_alloca_dst_id, std::nullopt
            );

            builder.append_instruction(store_instruction);

            // set the ir operand to be the pointer/address
            ast_param_symbol->set_ir_operand_id(ir_alloca_dst_id);
          }

          // generate the return operand and return block.

          // if the return type is not void, an address for the return value is
          // needed.
          if (!ast_func_ret_type->is_void()) {
            builder.curr_function->maybe_return_operand_id =
              builder.fetch_arbitrary_operand(
                builder.fetch_pointer_type(ir_func_ret_type)
              );

            auto alloca_instruction = builder.fetch_alloca_instruction(
              builder.curr_function->maybe_return_operand_id.value(),
              ir_func_ret_type, std::nullopt, std::nullopt, std::nullopt
            );

            builder.prepend_instruction_to_curr_function(alloca_instruction);
          }

          auto ir_func_return_block = builder.fetch_basic_block();
          builder.append_basic_block(ir_func_return_block);
          builder.curr_function->maybe_return_block = ir_func_return_block;
          
          builder.set_curr_basic_block(ir_func_return_block);

          if (!ast_func_ret_type->is_void()) {
            // if the return type is not void, the return value need to be
            // loaded from the memory.
            auto ir_loaded_return_operand_id =
              builder.fetch_arbitrary_operand(ir_func_ret_type);

            auto load_instruction = builder.fetch_load_instruction(
              ir_loaded_return_operand_id,
              builder.curr_function->maybe_return_operand_id.value(),
              std::nullopt
            );

            builder.append_instruction(load_instruction);

            auto ret_instruction =
              builder.fetch_ret_instruction(ir_loaded_return_operand_id);

            builder.append_instruction(ret_instruction);
          } else {
            // otherwise just return.
            auto ret_instruction = builder.fetch_ret_instruction(std::nullopt);
            builder.append_instruction(ret_instruction);
          }

          // set the block back to the entry block
          builder.set_curr_basic_block(ir_func_entry_block);

          // generate the statements in the body
          for (const auto& stmt : ast_block.stmts) {
            irgen_stmt(stmt, ast_block.symtable, builder);
          }

          builder.curr_function->add_terminators(builder);
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
      [symtable, &builder](const frontend::ast::stmt::Assign& kind) {
        auto ir_lhs_operand_id = irgen_expr(kind.lhs, symtable, builder, true);
        auto ir_rhs_operand_id = irgen_expr(kind.rhs, symtable, builder, false);

        auto store_instruction = builder.fetch_store_instruction(
          ir_rhs_operand_id, ir_lhs_operand_id, std::nullopt
        );

        builder.append_instruction(store_instruction);
      },
      [symtable, &builder](const frontend::ast::stmt::Return& kind) {

      },
      [&builder](const auto&) {
        // TODO
      },
    },
    stmt->kind
  );
}

IrOperandID irgen_expr(
  AstExprPtr expr,
  AstSymbolTablePtr symtable,
  IrBuilder& builder,
  bool is_lval
) {
  return std::visit(
    overloaded{
      [&builder](const frontend::ast::expr::Constant& kind) {
        auto ir_constant = irgen_comptime_value(kind.value, builder);
        auto ir_constant_operand_id =
          builder.fetch_operand(ir_constant->type, ir_constant);
        return ir_constant_operand_id;
      },
      [symtable, &builder, is_lval](const frontend::ast::expr::Binary& kind) {
        IrOperandID ir_lhs_operand_id;
        if (kind.op == AstBinaryOp::Index) {
          ir_lhs_operand_id = irgen_expr(kind.lhs, symtable, builder, true);
        } else {
          ir_lhs_operand_id = irgen_expr(kind.lhs, symtable, builder, false);
        }
        auto ir_rhs_operand_id = irgen_expr(kind.rhs, symtable, builder, false);

        auto symbol = kind.symbol;

        auto ast_dst_type = symbol->type;
        auto ast_lhs_type = kind.lhs->get_type();
        auto ast_rhs_type = kind.rhs->get_type();

        auto ir_dst_operand_id = builder.fetch_arbitrary_operand(
          irgen_type(ast_dst_type, builder).value()
        );

        symbol->set_ir_operand_id(ir_dst_operand_id);

        switch (kind.op) {
          case AstBinaryOp::Add: {
            IrBinaryOp ir_op;
            if (ast_dst_type->is_float()) {
              ir_op = IrBinaryOp::FAdd;
            } else {
              ir_op = IrBinaryOp::Add;
            }
            auto instruction = builder.fetch_binary_instruction(
              ir_op, ir_dst_operand_id, ir_lhs_operand_id, ir_rhs_operand_id
            );
            builder.append_instruction(instruction);
            break;
          }
          case AstBinaryOp::Sub: {
            IrBinaryOp ir_op;
            if (ast_dst_type->is_float()) {
              ir_op = IrBinaryOp::FSub;
            } else {
              ir_op = IrBinaryOp::Sub;
            }
            auto instruction = builder.fetch_binary_instruction(
              ir_op, ir_dst_operand_id, ir_lhs_operand_id, ir_rhs_operand_id
            );
            builder.append_instruction(instruction);
            break;
          }
          case AstBinaryOp::Div: {
            IrBinaryOp ir_op;
            if (ast_dst_type->is_float()) {
              ir_op = IrBinaryOp::FDiv;
            } else {
              ir_op = IrBinaryOp::SDiv;
            }
            auto instruction = builder.fetch_binary_instruction(
              ir_op, ir_dst_operand_id, ir_lhs_operand_id, ir_rhs_operand_id
            );
            builder.append_instruction(instruction);
            break;
          }
          case AstBinaryOp::Mul: {
            IrBinaryOp ir_op;
            if (ast_dst_type->is_float()) {
              ir_op = IrBinaryOp::FMul;
            } else {
              ir_op = IrBinaryOp::Mul;
            }
            auto instruction = builder.fetch_binary_instruction(
              ir_op, ir_dst_operand_id, ir_lhs_operand_id, ir_rhs_operand_id
            );
            builder.append_instruction(instruction);
            break;
          }
          case AstBinaryOp::Mod: {
            IrBinaryOp ir_op = IrBinaryOp::SRem;
            auto instruction = builder.fetch_binary_instruction(
              ir_op, ir_dst_operand_id, ir_lhs_operand_id, ir_rhs_operand_id
            );
            builder.append_instruction(instruction);
            break;
          }
          case AstBinaryOp::Lt: {
            if (ast_lhs_type->is_float()) {
              auto instruction = builder.fetch_fcmp_instruction(
                IrFCmpCond::Olt, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            } else {
              auto instruction = builder.fetch_icmp_instruction(
                IrICmpCond::Slt, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            }
            break;
          }
          case AstBinaryOp::Le: {
            if (ast_lhs_type->is_float()) {
              auto instruction = builder.fetch_fcmp_instruction(
                IrFCmpCond::Ole, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            } else {
              auto instruction = builder.fetch_icmp_instruction(
                IrICmpCond::Sle, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            }
            break;
          }
          case AstBinaryOp::Gt: {
            if (ast_lhs_type->is_float()) {
              auto instruction = builder.fetch_fcmp_instruction(
                IrFCmpCond::Olt, ir_dst_operand_id, ir_rhs_operand_id,
                ir_lhs_operand_id
              );
              builder.append_instruction(instruction);
            } else {
              auto instruction = builder.fetch_icmp_instruction(
                IrICmpCond::Slt, ir_dst_operand_id, ir_rhs_operand_id,
                ir_lhs_operand_id
              );
              builder.append_instruction(instruction);
            }
            break;
          }
          case AstBinaryOp::Ge: {
            if (ast_lhs_type->is_float()) {
              auto instruction = builder.fetch_fcmp_instruction(
                IrFCmpCond::Ole, ir_dst_operand_id, ir_rhs_operand_id,
                ir_lhs_operand_id
              );
              builder.append_instruction(instruction);
            } else {
              auto instruction = builder.fetch_icmp_instruction(
                IrICmpCond::Sle, ir_dst_operand_id, ir_rhs_operand_id,
                ir_lhs_operand_id
              );
              builder.append_instruction(instruction);
            }
            break;
          }
          case AstBinaryOp::Eq: {
            if (ast_lhs_type->is_float()) {
              auto instruction = builder.fetch_fcmp_instruction(
                IrFCmpCond::Oeq, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            } else {
              auto instruction = builder.fetch_icmp_instruction(
                IrICmpCond::Eq, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            }
            break;
          }
          case AstBinaryOp::Ne: {
            if (ast_lhs_type->is_float()) {
              auto instruction = builder.fetch_fcmp_instruction(
                IrFCmpCond::One, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            } else {
              auto instruction = builder.fetch_icmp_instruction(
                IrICmpCond::Ne, ir_dst_operand_id, ir_lhs_operand_id,
                ir_rhs_operand_id
              );
              builder.append_instruction(instruction);
            }
            break;
          }
          case AstBinaryOp::LogicalAnd:
          case AstBinaryOp::LogicalOr: {
            std::string error_message =
              "Error: Logical and/or shall be expanded to be short-circuit "
              "evaluation.";
            throw std::runtime_error(error_message);
            break;
          }
          case AstBinaryOp::Index: {
            auto ir_lhs_operand =
              builder.context.get_operand(ir_lhs_operand_id);
            auto ir_rhs_operand =
              builder.context.get_operand(ir_rhs_operand_id);

            auto ir_gep_dst_type = builder.fetch_pointer_type(
              irgen_type(ast_dst_type, builder).value()
            );

            auto ir_gep_dst_operand_id =
              builder.fetch_arbitrary_operand(ir_gep_dst_type);

            auto ir_gep_index_0 =
              builder.fetch_constant_operand(builder.fetch_i32_type(), (int)0);

            auto gep_instruction = builder.fetch_getelementptr_instruction(
              ir_gep_dst_operand_id, irgen_type(ast_lhs_type, builder).value(),
              ir_lhs_operand_id, {ir_gep_index_0, ir_rhs_operand_id}
            );

            builder.append_instruction(gep_instruction);

            if (!is_lval) {
              auto ir_load_dst_operand_id = builder.fetch_arbitrary_operand(
                irgen_type(ast_dst_type, builder).value()
              );
              auto load_instruction = builder.fetch_load_instruction(
                ir_load_dst_operand_id, ir_gep_dst_operand_id, std::nullopt
              );
              builder.append_instruction(load_instruction);
              symbol->set_ir_operand_id(ir_load_dst_operand_id);
              ir_dst_operand_id = ir_load_dst_operand_id;
            } else {
              symbol->set_ir_operand_id(ir_gep_dst_operand_id);
              ir_dst_operand_id = ir_gep_dst_operand_id;
            }

            break;
          }
          default:
            // unreachable.
            break;
        }
        return ir_dst_operand_id;
      },
      [symtable, &builder,
       is_lval](const frontend::ast::expr::Identifier& kind) -> IrOperandID {
        auto ast_symbol = kind.symbol;

        IrOperandID ir_operand_id;

        switch (ast_symbol->scope) {
          case AstScope::Global:
          case AstScope::Local:
          case AstScope::Param: {
            if (ast_symbol->maybe_ir_operand_id.has_value()) {
              ir_operand_id = ast_symbol->maybe_ir_operand_id.value();
            } else {
              std::string error_message =
                "Error: identifier `" + ast_symbol->name + "` is not defined.";
              throw std::runtime_error(error_message);
            }
            // If the identifier is not used as a lval, the actuall value needs
            // to be loaded.
            if (!is_lval) {
              if (ast_symbol->type->is_array()) {
                std::string error_message =
                  "Error: array type cannot be loaded directly.";
                throw std::runtime_error(error_message);
              }

              auto ir_load_dst_operand_id = builder.fetch_arbitrary_operand(
                irgen_type(ast_symbol->type, builder).value()
              );

              auto load_instruction = builder.fetch_load_instruction(
                ir_load_dst_operand_id, ir_operand_id, std::nullopt
              );
              builder.append_instruction(load_instruction);

              ir_operand_id = ir_load_dst_operand_id;
            }
            break;
          }
          case AstScope::Temp: {
            std::string error_message =
              "Error: using temporary symbol as identifier.";
            throw std::runtime_error(error_message);
          }
          default:
            // unreachable
            break;
        }
        return ir_operand_id;
      },
      [symtable, &builder](const frontend::ast::expr::Cast& kind) {
        auto ast_from_type = kind.expr->get_type();
        auto ast_to_type = kind.type;

        auto ir_operand_id = irgen_expr(kind.expr, symtable, builder, false);

        IrOperandID ir_dst_operand_id;

        if (ast_from_type == ast_to_type) {
          return ir_operand_id;
        }

        if (ast_from_type->is_int() && ast_to_type->is_float()) {
          ir_dst_operand_id = builder.fetch_arbitrary_operand(
            irgen_type(ast_to_type, builder).value()
          );
          auto cast_instruction = builder.fetch_cast_instruction(
            IrCastOp::SIToFP, ir_dst_operand_id, ir_operand_id
          );
          builder.append_instruction(cast_instruction);
        } else if (ast_from_type->is_float() && ast_to_type->is_int()) {
          ir_dst_operand_id = builder.fetch_arbitrary_operand(
            irgen_type(ast_to_type, builder).value()
          );
          auto cast_instruction = builder.fetch_cast_instruction(
            IrCastOp::FPToSI, ir_dst_operand_id, ir_operand_id
          );
          builder.append_instruction(cast_instruction);
        } else if (ast_from_type->is_bool() && ast_to_type->is_int()) {
          ir_dst_operand_id = builder.fetch_arbitrary_operand(
            irgen_type(ast_to_type, builder).value()
          );
          auto cast_instruction = builder.fetch_cast_instruction(
            IrCastOp::ZExt, ir_dst_operand_id, ir_operand_id
          );
          builder.append_instruction(cast_instruction);
        } else if (ast_from_type->is_bool() && ast_to_type->is_float()) {
          // first cast it to i32
          ir_dst_operand_id =
            builder.fetch_arbitrary_operand(builder.fetch_i32_type());
          auto cast_instruction = builder.fetch_cast_instruction(
            IrCastOp::ZExt, ir_dst_operand_id, ir_operand_id
          );
          builder.append_instruction(cast_instruction);
          // then cast it to float
          ir_dst_operand_id = builder.fetch_arbitrary_operand(
            irgen_type(ast_to_type, builder).value()
          );
          cast_instruction = builder.fetch_cast_instruction(
            IrCastOp::SIToFP, ir_dst_operand_id, ir_operand_id
          );
          builder.append_instruction(cast_instruction);
        } else {
          std::string error_message = "Error: unsupported cast expression.";
          throw std::runtime_error(error_message);
        }
        return ir_dst_operand_id;
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
    auto length = std::get<frontend::type::Array>(type->kind).length;

    return builder.fetch_array_type(
      length, irgen_type(element_type, builder).value()
    );

  } else {
    std::string error_message = "Erro: cannot generate a function type in ir.";
    std::cerr << error_message << std::endl;
    return std::nullopt;
  }
}

}  // namespace syc