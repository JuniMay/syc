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
                builder.fetch_pointer_type()
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
                if (ast_expr->is_initializer_list()) {
                  bool memset_init = false;
                  if (ast_type->is_array() 
                  && ast_type->get_root_element_type().value()->is_int()) {
                    auto arr_size = ast_expr->get_type()->get_size() / 32;
                    if (arr_size >= 8) {
                      // if the size of array is greater than 8, use memset to initialize
                      auto ir_cast_dest_operand_id = builder.fetch_arbitrary_operand(
                        builder.fetch_pointer_type(
                        )
                      );
                      auto call_cast_instruction = builder.fetch_cast_instruction(
                        IrCastOp::BitCast,
                        ir_cast_dest_operand_id,
                        ir_dst_operand_id
                      );
                      builder.append_instruction(call_cast_instruction);
                      auto call_memset_instruction = builder.fetch_call_instruction(
                        std::nullopt,
                        "__builtin_fill_zero",
                        {
                          ir_cast_dest_operand_id,
                          builder.fetch_constant_operand(
                            builder.fetch_i32_type(), (int)arr_size
                          )
                        }
                      );
                      builder.append_instruction(call_memset_instruction);
                      memset_init = true;
                    }
                  }
                  auto ir_ptr_id = ir_dst_operand_id;

                  auto curr_ast_type = ast_type;
                  while (curr_ast_type->is_array()) {
                    auto ast_elem_type =
                      curr_ast_type->get_element_type().value();
                    auto ir_tmp_ptr_id = builder.fetch_arbitrary_operand(
                      builder.fetch_pointer_type(
                      )
                    );
                    auto gep_instruction =
                      builder.fetch_getelementptr_instruction(
                        ir_tmp_ptr_id,
                        irgen_type(curr_ast_type, builder).value(), ir_ptr_id,
                        {
                          builder.fetch_constant_operand(
                            builder.fetch_i32_type(), (int)0
                          ),
                          builder.fetch_constant_operand(
                            builder.fetch_i32_type(), (int)0
                          ),
                        }
                      );
                    builder.append_instruction(gep_instruction);

                    curr_ast_type = ast_elem_type;
                    ir_ptr_id = ir_tmp_ptr_id;
                  }

                  std::queue<AstExprPtr> ast_expr_queue;

                  ast_expr_queue.push(ast_expr);

                  size_t shift_amount = 0;
                  while (!ast_expr_queue.empty()) {
                    auto ast_expr = ast_expr_queue.front();
                    ast_expr_queue.pop();

                    if (ast_expr->is_initializer_list()) {
                      auto ast_initializer_list =
                        std::get<frontend::ast::expr::InitializerList>(
                          ast_expr->kind
                        );
                      if (ast_initializer_list.is_zeroinitializer) {
                        auto ast_type = ast_expr->get_type();
                        auto ast_elem_type =
                          ast_type->get_element_type().value();
                        auto length =
                          ast_type->get_size() / ast_elem_type->get_size();

                        for (size_t i = 0; i < length; ++i) {
                          AstExprPtr sub_expr;

                          if (ast_elem_type->is_array()) {
                            sub_expr =
                              frontend::ast::create_initializer_list_expr({});

                            std::get<frontend::ast::expr::InitializerList>(
                              sub_expr->kind
                            )
                              .type = ast_elem_type;

                            std::get<frontend::ast::expr::InitializerList>(
                              sub_expr->kind
                            )
                              .is_zeroinitializer = true;

                          } else {
                            auto ast_zero_constant =
                              frontend::create_zero_comptime_value(ast_elem_type
                              );

                            sub_expr = frontend::ast::create_constant_expr(
                              ast_zero_constant
                            );
                          }

                          ast_expr_queue.push(sub_expr);
                        }

                      } else {
                        for (auto sub_expr : ast_initializer_list.init_list) {
                          ast_expr_queue.push(sub_expr);
                        }
                      }
                    } else {
                      auto ir_value_id =
                        irgen_expr(ast_expr, symtable, builder, false).value();

                      if (memset_init) {
                        auto value = std::get_if<IrConstantPtr>(&builder.context.get_operand(ir_value_id)->kind);
                        if (value != nullptr && (*value)->is_zero()) {
                          // if the array is initialized by memset, 
                          // we don't need to store the value
                          shift_amount += 1;
                          continue;
                        }

                        auto ir_tmp_ptr_id = builder.fetch_arbitrary_operand(
                          builder.fetch_pointer_type(
                          )
                        );
                        auto gep_instruction =
                          builder.fetch_getelementptr_instruction(
                            ir_tmp_ptr_id,
                            irgen_type(curr_ast_type, builder).value(), ir_ptr_id,
                            {
                              builder.fetch_constant_operand(
                                builder.fetch_i32_type(), (int)shift_amount
                              ),
                            }
                          );
                        shift_amount = 1;
                        builder.append_instruction(gep_instruction);

                        ir_ptr_id = ir_tmp_ptr_id;
                        auto store_instruction = builder.fetch_store_instruction(
                          ir_value_id, ir_ptr_id, std::nullopt
                        );
                        builder.append_instruction(store_instruction);

                      } else {
                        auto store_instruction = builder.fetch_store_instruction(
                          ir_value_id, ir_ptr_id, std::nullopt
                        );

                        builder.append_instruction(store_instruction);

                        auto ir_tmp_ptr_id = builder.fetch_arbitrary_operand(
                          builder.fetch_pointer_type(
                          )
                        );
                        auto gep_instruction =
                          builder.fetch_getelementptr_instruction(
                            ir_tmp_ptr_id,
                            irgen_type(curr_ast_type, builder).value(), ir_ptr_id,
                            {
                              builder.fetch_constant_operand(
                                builder.fetch_i32_type(), (int)1
                              ),
                            }
                          );
                        builder.append_instruction(gep_instruction);
                        ir_ptr_id = ir_tmp_ptr_id;
                      }
                    }
                  }
                } else {
                  auto ir_init_operand_id =
                    irgen_expr(ast_expr, symtable, builder).value();
                  auto store_instruction = builder.fetch_store_instruction(
                    ir_init_operand_id, ir_dst_operand_id, std::nullopt
                  );
                  builder.append_instruction(store_instruction);
                }
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
              builder.fetch_pointer_type()
            );

            auto alloca_instruction = builder.fetch_alloca_instruction(
              ir_alloca_dst_id, ir_param_operand->type, std::nullopt,
              std::nullopt, std::nullopt, true
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
                builder.fetch_pointer_type()
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
          builder.curr_function->remove_unused_basic_blocks(builder.context);
        }
      },
      [&builder](const frontend::ast::stmt::Block& kind) {
        for (auto stmt : kind.stmts) {
          irgen_stmt(stmt, kind.symtable, builder);
        }
      },
      [symtable, &builder](const frontend::ast::stmt::Assign& kind) {
        auto ir_lhs_operand_id =
          irgen_expr(kind.lhs, symtable, builder, true).value();
        auto ir_rhs_operand_id =
          irgen_expr(kind.rhs, symtable, builder, false).value();

        auto store_instruction = builder.fetch_store_instruction(
          ir_rhs_operand_id, ir_lhs_operand_id, std::nullopt
        );

        builder.append_instruction(store_instruction);
      },
      [symtable, &builder](const frontend::ast::stmt::Return& kind) {
        if (kind.maybe_expr.has_value()) {
          auto ir_operand_id =
            irgen_expr(kind.maybe_expr.value(), symtable, builder, false)
              .value();
          auto store_instruction = builder.fetch_store_instruction(
            ir_operand_id,
            builder.curr_function->maybe_return_operand_id.value(), std::nullopt
          );
          builder.append_instruction(store_instruction);
        }
        auto br_instruction = builder.fetch_br_instruction(
          builder.curr_function->maybe_return_block.value()->id
        );
        builder.append_instruction(br_instruction);
      },
      [symtable, &builder](const frontend::ast::stmt::If& kind) {
        auto ir_then_basic_block = builder.fetch_basic_block();
        auto maybe_ir_else_basic_block =
          kind.maybe_else_stmt.has_value()
            ? std::make_optional(builder.fetch_basic_block())
            : std::nullopt;

        auto ir_tail_basic_block = builder.fetch_basic_block();

        // generate condition in the previous basic block.
        auto ir_cond_operand_id =
          irgen_expr(kind.cond, symtable, builder, false).value();

        // if the condition is true, jump to the then stmt.
        // otherwise, if there is an else stmt, jump to the else stmt, or jump
        // to the tail.
        auto condbr_instruction = builder.fetch_condbr_instruction(
          ir_cond_operand_id, ir_then_basic_block->id,
          maybe_ir_else_basic_block.has_value()
            ? maybe_ir_else_basic_block.value()->id
            : ir_tail_basic_block->id
        );
        builder.append_instruction(condbr_instruction);

        // generate the then stmt.
        builder.append_basic_block(ir_then_basic_block);
        builder.set_curr_basic_block(ir_then_basic_block);
        auto maybe_block = kind.then_stmt->as_block();
        if (maybe_block.has_value()) {
          auto block = maybe_block.value();
          for (auto stmt : block.stmts) {
            irgen_stmt(stmt, block.symtable, builder);
            if (builder.curr_basic_block->has_terminator()) {
              break;
            }
          }
        } else {
          irgen_stmt(kind.then_stmt, symtable, builder);
        }

        if (!builder.curr_basic_block->has_terminator()) {
          // after finishing the then stmt, jump to the tail.
          auto br_instruction =
            builder.fetch_br_instruction(ir_tail_basic_block->id);
          builder.append_instruction(br_instruction);
        }

        // generate the else stmt (if exists).
        if (maybe_ir_else_basic_block.has_value()) {
          auto ir_else_basic_block = maybe_ir_else_basic_block.value();
          builder.append_basic_block(ir_else_basic_block);
          builder.set_curr_basic_block(ir_else_basic_block);

          auto ast_else_stmt = kind.maybe_else_stmt.value();
          auto maybe_block = ast_else_stmt->as_block();
          if (maybe_block.has_value()) {
            auto block = maybe_block.value();
            for (auto stmt : block.stmts) {
              irgen_stmt(stmt, block.symtable, builder);
              if (builder.curr_basic_block->has_terminator()) {
                break;
              }
            }
          } else {
            irgen_stmt(ast_else_stmt, symtable, builder);
          }
          if (!builder.curr_basic_block->has_terminator()) {
            auto br_instruction =
              builder.fetch_br_instruction(ir_tail_basic_block->id);
            builder.append_instruction(br_instruction);
          }
        }

        // just append the tail and set it to be the current block.
        builder.append_basic_block(ir_tail_basic_block);
        builder.set_curr_basic_block(ir_tail_basic_block);
      },
      [symtable, &builder](const frontend::ast::stmt::While& kind) {
        auto ir_cond_basic_block = builder.fetch_basic_block();
        auto ir_body_basic_block = builder.fetch_basic_block();
        auto ir_tail_basic_block = builder.fetch_basic_block();

        builder.while_cond_basic_block_stack.push(ir_cond_basic_block);
        builder.while_tail_basic_block_stack.push(ir_tail_basic_block);

        builder.append_basic_block(ir_cond_basic_block);
        builder.set_curr_basic_block(ir_cond_basic_block);
        auto ir_cond_operand_id =
          irgen_expr(kind.cond, symtable, builder, false).value();
        auto condbr_instruction = builder.fetch_condbr_instruction(
          ir_cond_operand_id, ir_body_basic_block->id, ir_tail_basic_block->id
        );
        builder.append_instruction(condbr_instruction);

        builder.append_basic_block(ir_body_basic_block);
        builder.set_curr_basic_block(ir_body_basic_block);

        auto maybe_block = kind.body->as_block();
        if (maybe_block.has_value()) {
          auto block = maybe_block.value();
          for (auto stmt : block.stmts) {
            irgen_stmt(stmt, block.symtable, builder);
            if (builder.curr_basic_block->has_terminator()) {
              break;
            }
          }
        } else {
          irgen_stmt(kind.body, symtable, builder);
        }

        if (!builder.curr_basic_block->has_terminator()) {
          auto br_instruction =
            builder.fetch_br_instruction(ir_cond_basic_block->id);
          builder.append_instruction(br_instruction);
        }

        builder.append_basic_block(ir_tail_basic_block);
        builder.set_curr_basic_block(ir_tail_basic_block);

        builder.while_cond_basic_block_stack.pop();
        builder.while_tail_basic_block_stack.pop();
      },
      [symtable, &builder](const frontend::ast::stmt::Break& kind) {
        auto br_instruction = builder.fetch_br_instruction(
          builder.while_tail_basic_block_stack.top()->id
        );
        builder.append_instruction(br_instruction);
      },
      [symtable, &builder](const frontend::ast::stmt::Continue& kind) {
        auto br_instruction = builder.fetch_br_instruction(
          builder.while_cond_basic_block_stack.top()->id
        );
        builder.append_instruction(br_instruction);
      },
      [symtable, &builder](const frontend::ast::stmt::Expr& kind) {
        irgen_expr(kind.expr, symtable, builder, false);
      },
      [&builder](const auto&) {},
    },
    stmt->kind
  );
}

std::optional<IrOperandID> irgen_expr(
  AstExprPtr expr,
  AstSymbolTablePtr symtable,
  IrBuilder& builder,
  bool use_address
) {
  return std::visit(
    overloaded{
      [&builder](const frontend::ast::expr::Constant& kind
      ) -> std::optional<IrOperandID> {
        auto ir_constant = irgen_comptime_value(kind.value, builder);
        auto ir_constant_operand_id =
          builder.fetch_operand(ir_constant->type, ir_constant);
        return ir_constant_operand_id;
      },
      [symtable, &builder, use_address](const frontend::ast::expr::Binary& kind
      ) -> std::optional<IrOperandID> {
        auto symbol = kind.symbol;

        auto ast_dst_type = symbol->type;
        auto ast_lhs_type = kind.lhs->get_type();
        auto ast_rhs_type = kind.rhs->get_type();

        IrOperandID ir_dst_operand_id;

        switch (kind.op) {
          case AstBinaryOp::Add: {
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

            IrBinaryOp ir_op = IrBinaryOp::SRem;
            auto instruction = builder.fetch_binary_instruction(
              ir_op, ir_dst_operand_id, ir_lhs_operand_id, ir_rhs_operand_id
            );
            builder.append_instruction(instruction);
            break;
          }
          case AstBinaryOp::Lt: {
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, false).value();
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            ir_dst_operand_id = builder.fetch_arbitrary_operand(
              irgen_type(ast_dst_type, builder).value()
            );
            symbol->set_ir_operand_id(ir_dst_operand_id);

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
          case AstBinaryOp::LogicalAnd: {
            // pointer of the address that store the result.
            auto ir_dst_pointer_operand_id = builder.fetch_arbitrary_operand(
              builder.fetch_pointer_type()
            );
            auto alloca_instruction = builder.fetch_alloca_instruction(
              ir_dst_pointer_operand_id, builder.fetch_i1_type(), std::nullopt,
              std::nullopt, std::nullopt
            );
            builder.prepend_instruction_to_curr_function(alloca_instruction);

            // block to generate the rhs expression
            auto ir_rhs_basic_block = builder.fetch_basic_block();
            // block to store `true` into the result.
            auto ir_true_basic_block = builder.fetch_basic_block();
            // block to store `false` into the result.
            auto ir_false_basic_block = builder.fetch_basic_block();
            // end of the evaluation.
            auto ir_tail_basic_block = builder.fetch_basic_block();

            // generate lhs with short-circuit evaluation.
            // lhs is generated in the previous basic block.
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, true).value();
            // if lhs is true, evaluate the rhs, otherwise the result is false.
            auto condbr_instruction = builder.fetch_condbr_instruction(
              ir_lhs_operand_id, ir_rhs_basic_block->id,
              ir_false_basic_block->id
            );
            builder.append_instruction(condbr_instruction);

            // generate rhs with short-circuit evaluation.
            builder.append_basic_block(ir_rhs_basic_block);
            builder.set_curr_basic_block(ir_rhs_basic_block);
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, true).value();
            // if rhs is true, the result is true, otherwise the result is
            // false.
            condbr_instruction = builder.fetch_condbr_instruction(
              ir_rhs_operand_id, ir_true_basic_block->id,
              ir_false_basic_block->id
            );
            builder.append_instruction(condbr_instruction);

            // store `true` to the result.
            builder.append_basic_block(ir_true_basic_block);
            builder.set_curr_basic_block(ir_true_basic_block);
            auto store_instruction = builder.fetch_store_instruction(
              builder.fetch_constant_operand(builder.fetch_i1_type(), (int)1),
              ir_dst_pointer_operand_id, std::nullopt
            );
            builder.append_instruction(store_instruction);
            // after the result is stored, jump to the end of evaluation.
            auto br_instruction =
              builder.fetch_br_instruction(ir_tail_basic_block->id);
            builder.append_instruction(br_instruction);

            // store `false` to the result.
            builder.append_basic_block(ir_false_basic_block);
            builder.set_curr_basic_block(ir_false_basic_block);
            store_instruction = builder.fetch_store_instruction(
              builder.fetch_constant_operand(builder.fetch_i1_type(), (int)0),
              ir_dst_pointer_operand_id, std::nullopt
            );
            builder.append_instruction(store_instruction);
            // after the result is stored, jump to the end of evaluation.
            br_instruction =
              builder.fetch_br_instruction(ir_tail_basic_block->id);
            builder.append_instruction(br_instruction);

            // set current block to the end of evaluation.
            builder.append_basic_block(ir_tail_basic_block);
            builder.set_curr_basic_block(ir_tail_basic_block);

            // load the result from the memory as an operand.
            ir_dst_operand_id =
              builder.fetch_arbitrary_operand(builder.fetch_i1_type());
            auto load_instruction = builder.fetch_load_instruction(
              ir_dst_operand_id, ir_dst_pointer_operand_id, std::nullopt
            );
            builder.append_instruction(load_instruction);
            break;
          }
          case AstBinaryOp::LogicalOr: {
            // pointer of the address that store the result.
            auto ir_dst_pointer_operand_id = builder.fetch_arbitrary_operand(
              builder.fetch_pointer_type()
            );
            auto alloca_instruction = builder.fetch_alloca_instruction(
              ir_dst_pointer_operand_id, builder.fetch_i1_type(), std::nullopt,
              std::nullopt, std::nullopt
            );
            builder.prepend_instruction_to_curr_function(alloca_instruction);

            // block to generate the rhs expression
            auto ir_rhs_basic_block = builder.fetch_basic_block();
            // block to store `true` into the result.
            auto ir_true_basic_block = builder.fetch_basic_block();
            // block to store `false` into the result.
            auto ir_false_basic_block = builder.fetch_basic_block();
            // end of the evaluation.
            auto ir_tail_basic_block = builder.fetch_basic_block();

            // generate lhs with short-circuit evaluation.
            // lhs is generated in the previous basic block.
            auto ir_lhs_operand_id =
              irgen_expr(kind.lhs, symtable, builder, true).value();
            // if lhs is true, the result is true.
            // this is the only difference between logical and and logical or
            auto condbr_instruction = builder.fetch_condbr_instruction(
              ir_lhs_operand_id, ir_true_basic_block->id, ir_rhs_basic_block->id
            );
            builder.append_instruction(condbr_instruction);

            // generate rhs with short-circuit evaluation.
            builder.append_basic_block(ir_rhs_basic_block);
            builder.set_curr_basic_block(ir_rhs_basic_block);
            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, true).value();
            // if rhs is true, the result is true, otherwise the result is
            // false.
            condbr_instruction = builder.fetch_condbr_instruction(
              ir_rhs_operand_id, ir_true_basic_block->id,
              ir_false_basic_block->id
            );
            builder.append_instruction(condbr_instruction);

            // store `true` to the result.
            builder.append_basic_block(ir_true_basic_block);
            builder.set_curr_basic_block(ir_true_basic_block);
            auto store_instruction = builder.fetch_store_instruction(
              builder.fetch_constant_operand(builder.fetch_i1_type(), (int)1),
              ir_dst_pointer_operand_id, std::nullopt
            );
            builder.append_instruction(store_instruction);
            // after the result is stored, jump to the end of evaluation.
            auto br_instruction =
              builder.fetch_br_instruction(ir_tail_basic_block->id);
            builder.append_instruction(br_instruction);

            // store `false` to the result.
            builder.append_basic_block(ir_false_basic_block);
            builder.set_curr_basic_block(ir_false_basic_block);
            store_instruction = builder.fetch_store_instruction(
              builder.fetch_constant_operand(builder.fetch_i1_type(), (int)0),
              ir_dst_pointer_operand_id, std::nullopt
            );
            builder.append_instruction(store_instruction);
            // after the result is stored, jump to the end of evaluation.
            br_instruction =
              builder.fetch_br_instruction(ir_tail_basic_block->id);
            builder.append_instruction(br_instruction);

            // set current block to the end of evaluation.
            builder.append_basic_block(ir_tail_basic_block);
            builder.set_curr_basic_block(ir_tail_basic_block);

            // load the result from the memory as an operand.
            ir_dst_operand_id =
              builder.fetch_arbitrary_operand(builder.fetch_i1_type());
            auto load_instruction = builder.fetch_load_instruction(
              ir_dst_operand_id, ir_dst_pointer_operand_id, std::nullopt
            );
            builder.append_instruction(load_instruction);
            break;
          }
          case AstBinaryOp::Index: {
            IrOperandID ir_lhs_operand_id;

            IrTypePtr basis_type;

            if (ast_lhs_type->is_array()) {
              ir_lhs_operand_id =
                irgen_expr(kind.lhs, symtable, builder, true).value();
              basis_type = irgen_type(ast_lhs_type, builder).value();
            } else if (ast_lhs_type->is_pointer()) {
              ir_lhs_operand_id =
                irgen_expr(kind.lhs, symtable, builder, false).value();
              basis_type =
                irgen_type(ast_lhs_type->get_value_type().value(), builder)
                  .value();
            } else {
              std::string error_message =
                "Error: indexing on non-array/pointer type.";
              throw std::runtime_error(error_message);
            }

            auto ir_rhs_operand_id =
              irgen_expr(kind.rhs, symtable, builder, false).value();

            if (use_address) {
              ir_dst_operand_id =
                builder.fetch_arbitrary_operand(builder.fetch_pointer_type(
                ));
              symbol->set_ir_operand_id(ir_dst_operand_id);

              if (ast_lhs_type->is_array()) {
                auto ir_zero_index_operand_id = builder.fetch_constant_operand(
                  builder.fetch_i32_type(), (int)0
                );
                auto gep_instruction = builder.fetch_getelementptr_instruction(
                  ir_dst_operand_id, basis_type, ir_lhs_operand_id,
                  {ir_zero_index_operand_id, ir_rhs_operand_id}
                );
                builder.append_instruction(gep_instruction);
              } else if (ast_lhs_type->is_pointer()) {
                auto gep_instruction = builder.fetch_getelementptr_instruction(
                  ir_dst_operand_id, basis_type, ir_lhs_operand_id,
                  {ir_rhs_operand_id}
                );
                builder.append_instruction(gep_instruction);
              }
            } else {
              ir_dst_operand_id = builder.fetch_arbitrary_operand(
                irgen_type(ast_dst_type, builder).value()
              );
              symbol->set_ir_operand_id(ir_dst_operand_id);

              if (ast_lhs_type->is_array()) {
                auto ir_zero_index_operand_id = builder.fetch_constant_operand(
                  builder.fetch_i32_type(), (int)0
                );
                auto ir_tmp_pointer_operand_id =
                  builder.fetch_arbitrary_operand(builder.fetch_pointer_type(
                  ));
                auto gep_instruction = builder.fetch_getelementptr_instruction(
                  ir_tmp_pointer_operand_id, basis_type, ir_lhs_operand_id,
                  {ir_zero_index_operand_id, ir_rhs_operand_id}
                );
                builder.append_instruction(gep_instruction);
                auto load_instruction = builder.fetch_load_instruction(
                  ir_dst_operand_id, ir_tmp_pointer_operand_id, std::nullopt
                );
                builder.append_instruction(load_instruction);
              } else if (ast_lhs_type->is_pointer()) {
                auto ir_tmp_pointer_operand_id =
                  builder.fetch_arbitrary_operand(builder.fetch_pointer_type(
                  ));
                auto gep_instruction = builder.fetch_getelementptr_instruction(
                  ir_tmp_pointer_operand_id, basis_type, ir_lhs_operand_id,
                  {ir_rhs_operand_id}
                );
                builder.append_instruction(gep_instruction);
                auto load_instruction = builder.fetch_load_instruction(
                  ir_dst_operand_id, ir_tmp_pointer_operand_id, std::nullopt
                );
                builder.append_instruction(load_instruction);
              }
            }
          }
          default:
            // unreachable.
            break;
        }
        return ir_dst_operand_id;
      },
      [symtable, &builder,
       use_address](const frontend::ast::expr::Identifier& kind
      ) -> std::optional<IrOperandID> {
        auto ast_symbol = kind.symbol;

        if (!ast_symbol->maybe_ir_operand_id.has_value()) {
          std::string error_message =
            "Error: symbol `" + ast_symbol->name + "` is not defined";
          throw std::runtime_error(error_message);
        }

        auto ast_type = ast_symbol->type;
        auto ir_type = irgen_type(ast_type, builder).value();

        IrOperandID ir_operand_id;

        if (use_address) {
          ir_operand_id = ast_symbol->maybe_ir_operand_id.value();
        } else {
          ir_operand_id = builder.fetch_arbitrary_operand(ir_type);
          auto load_instruction = builder.fetch_load_instruction(
            ir_operand_id, ast_symbol->maybe_ir_operand_id.value(), std::nullopt
          );
          builder.append_instruction(load_instruction);
        }

        return ir_operand_id;
      },
      [symtable, &builder](const frontend::ast::expr::Cast& kind
      ) -> std::optional<IrOperandID> {
        auto ast_from_type = kind.expr->get_type();
        auto ast_to_type = kind.type;

        auto ir_operand_id =
          irgen_expr(kind.expr, symtable, builder, false).value();

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
      [symtable, &builder](const frontend::ast::expr::Unary& kind
      ) -> std::optional<IrOperandID> {
        auto ir_src_operand_id =
          irgen_expr(kind.expr, symtable, builder, false).value();

        IrOperandID ir_dst_operand_id;

        switch (kind.op) {
          case AstUnaryOp::Pos:
            ir_dst_operand_id = ir_src_operand_id;
            break;
          case AstUnaryOp::Neg: {
            if (kind.expr->get_type()->is_int()) {
              ir_dst_operand_id =
                builder.fetch_arbitrary_operand(builder.fetch_i32_type());
              auto ir_zero_operand_id = builder.fetch_constant_operand(
                builder.fetch_i32_type(), (int)0
              );
              auto sub_instruction = builder.fetch_binary_instruction(
                IrBinaryOp::Sub, ir_dst_operand_id, ir_zero_operand_id,
                ir_src_operand_id
              );
              builder.append_instruction(sub_instruction);
            } else if (kind.expr->get_type()->is_float()) {
              ir_dst_operand_id =
                builder.fetch_arbitrary_operand(builder.fetch_float_type());
              auto ir_zero_operand_id = builder.fetch_constant_operand(
                builder.fetch_float_type(), (float)0.0
              );
              auto sub_instruction = builder.fetch_binary_instruction(
                IrBinaryOp::FSub, ir_dst_operand_id, ir_zero_operand_id,
                ir_src_operand_id
              );
              builder.append_instruction(sub_instruction);
            } else {
              std::string error_message =
                "Error: unsupported type for unary expression.";
              throw std::runtime_error(error_message);
            }
            break;
          }
          case AstUnaryOp::LogicalNot: {
            // This shall all be converted to bool in ast.
            ir_dst_operand_id =
              builder.fetch_arbitrary_operand(builder.fetch_i1_type());
            auto ir_zero_operand_id =
              builder.fetch_constant_operand(builder.fetch_i1_type(), (int)0);

            auto eq_instruction = builder.fetch_icmp_instruction(
              IrICmpCond::Eq, ir_dst_operand_id, ir_src_operand_id,
              ir_zero_operand_id
            );

            builder.append_instruction(eq_instruction);
            break;
          }
        }
        return ir_dst_operand_id;
      },
      [symtable, &builder](const frontend::ast::expr::Call& kind
      ) -> std::optional<IrOperandID> {
        auto ast_func_symbol = kind.func_symbol;

        std::optional<IrOperandID> maybe_ir_dst_operand_id = std::nullopt;

        if (!ast_func_symbol->type->get_ret_type().value()->is_void()) {
          maybe_ir_dst_operand_id = builder.fetch_arbitrary_operand(
            irgen_type(kind.symbol->type, builder).value()
          );
        }

        const auto& param_types =
          std::get<frontend::type::Function>(ast_func_symbol->type->kind)
            .param_types;

        std::vector<IrOperandID> ir_arg_id_list;

        for (size_t i = 0; i < kind.args.size(); ++i) {
          auto arg = kind.args[i];
          auto param_type = param_types[i];

          auto ir_param_type = irgen_type(param_type, builder).value();

          IrOperandID ir_arg_id;
          if (arg->get_type()->is_pointer()) {
            ir_arg_id = irgen_expr(arg, symtable, builder, false).value();
            if (arg->get_type() != param_type) {
              // If pointer types are not the same, perform a bitcast so the
              // type is compatible.
              auto ir_tmp_id = builder.fetch_arbitrary_operand(ir_param_type);
              auto cast_instruction = builder.fetch_cast_instruction(
                IrCastOp::BitCast, ir_tmp_id, ir_arg_id
              );
              builder.append_instruction(cast_instruction);
              ir_arg_id = ir_tmp_id;
            }
          } else if (arg->get_type()->is_array()) {
            auto ir_arr_id = irgen_expr(arg, symtable, builder, true).value();

            auto ir_arg_type = builder.fetch_pointer_type(
            );
            // pass the pointer to the first dimension of the array.
            ir_arg_id = builder.fetch_arbitrary_operand(ir_arg_type);
            // A gep instruction is required.
            auto gep_instruction = builder.fetch_getelementptr_instruction(
              ir_arg_id, irgen_type(arg->get_type(), builder).value(),
              ir_arr_id,
              {
                builder.fetch_constant_operand(
                  builder.fetch_i32_type(), (int)0
                ),
                builder.fetch_constant_operand(
                  builder.fetch_i32_type(), (int)0
                ),
              }
            );
            builder.append_instruction(gep_instruction);

            if (ir_arg_type != ir_param_type) {
              // If pointer types are not the same, perform a bitcast so the
              // type is compatible.
              auto ir_tmp_id = builder.fetch_arbitrary_operand(ir_param_type);
              auto cast_instruction = builder.fetch_cast_instruction(
                IrCastOp::BitCast, ir_tmp_id, ir_arg_id
              );
              builder.append_instruction(cast_instruction);
              ir_arg_id = ir_tmp_id;
            }

          } else {
            ir_arg_id = irgen_expr(arg, symtable, builder, false).value();
          }
          ir_arg_id_list.push_back(ir_arg_id);
        }
        auto call_instruction = builder.fetch_call_instruction(
          maybe_ir_dst_operand_id, kind.func_symbol->name, ir_arg_id_list
        );
        builder.append_instruction(call_instruction);
        return maybe_ir_dst_operand_id;
      },
      [](const auto& kind) -> std::optional<IrOperandID> { return 0; },
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