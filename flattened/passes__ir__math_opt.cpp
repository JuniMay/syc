#include "passes__ir__math_opt.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void math_opt(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    math_opt_function(function, builder);
  }
}

void math_opt_function(FunctionPtr function, Builder& builder) {
  auto math_opt_ctx = MathOptContext();
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    math_opt_block(curr_bb, builder, math_opt_ctx);
    curr_bb = curr_bb->next;
  }
  bool changed = true;
  while (changed) {
    changed = simplify(builder, math_opt_ctx);
    prune(builder, math_opt_ctx);
  }
}

void math_opt_block(
  BasicBlockPtr basic_block,
  Builder& builder,
  MathOptContext& math_opt_ctx
) {
  using namespace instruction;
  builder.set_curr_basic_block(basic_block);

  auto curr_instr = basic_block->head_instruction->next;
  while (curr_instr != basic_block->tail_instruction) {
    auto maybe_binary = curr_instr->as<Binary>();

    if (maybe_binary.has_value()) {
      auto binary = maybe_binary.value();
      auto op = binary.op;
      auto dst_id = binary.dst_id;
      auto lhs_id = binary.lhs_id;
      auto rhs_id = binary.rhs_id;

      auto lhs = builder.context.get_operand(lhs_id);
      auto rhs = builder.context.get_operand(rhs_id);

      // Make sure inside the tree rhs_id is constant (if any). Note that this
      // does not change the order of operands in IR.
      if (op == BinaryOp::Add || op == BinaryOp::Mul) {
        if (lhs->is_constant() && rhs->is_arbitrary()) {
          std::swap(lhs_id, rhs_id);
        }
      }

      math_opt_ctx.expr_forest[dst_id] = {op, lhs_id, rhs_id};
    }

    curr_instr = curr_instr->next;
  }
}

void prune(Builder& builder, MathOptContext& math_opt_ctx) {
  auto expr_forest_copy = math_opt_ctx.expr_forest;
  for (auto [dst_id, _] : expr_forest_copy) {
    auto dst = builder.context.get_operand(dst_id);
    if (dst->use_id_list.size() == 0) {
      math_opt_ctx.expr_forest.erase(dst_id);
    }
  }
}

bool simplify(Builder& builder, MathOptContext& math_opt_ctx) {
  using namespace instruction;
  // Traverse the expr_forest and try to simplify each expression.
  bool changed = false;
  auto expr_forest_copy = math_opt_ctx.expr_forest;
  for (auto [dst_id, triple] : expr_forest_copy) {
    auto [op, lhs_id, rhs_id] = triple;
    auto dst = builder.context.get_operand(dst_id);
    auto lhs = builder.context.get_operand(lhs_id);
    auto rhs = builder.context.get_operand(rhs_id);

    auto def_instr = builder.context.get_instruction(dst->maybe_def_id.value());
    auto def_bb = builder.context.get_basic_block(def_instr->parent_block_id);
    builder.set_curr_basic_block(def_bb);

    if (op == BinaryOp::Add) {
      if (lhs_id == rhs_id) {
        // a + a -> a * 2
        auto old_instr =
          builder.context.get_instruction(dst->maybe_def_id.value());

        auto constant_id = builder.fetch_constant_operand(dst->get_type(), 2);

        auto new_instr = builder.fetch_binary_instruction(
          BinaryOp::Mul, dst_id, lhs_id, constant_id
        );

        old_instr->insert_next(new_instr);
        old_instr->remove(builder.context);

        math_opt_ctx.expr_forest[dst_id] = {BinaryOp::Mul, lhs_id, constant_id};

        changed = true;
      } else if (lhs->is_arbitrary() && rhs->is_arbitrary()) {
        // Cases:
        // 1. (a * c) + a -> a * (c + 1)
        // 2. a + (a * c) -> a * (c + 1)
        // 3. (a * c) + (a * d) -> a * (c + d), maybe swaped
        // 4. (0 - x) + y -> y - x
        // 5. x + (0 - y) -> x - y

        if (math_opt_ctx.expr_forest.count(lhs_id) && math_opt_ctx.expr_forest.count(rhs_id)) {
          auto [lhs_op, lhs_lhs_id, lhs_rhs_id] =
            math_opt_ctx.expr_forest.at(lhs_id);
          auto [rhs_op, rhs_lhs_id, rhs_rhs_id] =
            math_opt_ctx.expr_forest.at(rhs_id);

          auto lhs_lhs = builder.context.get_operand(lhs_lhs_id);
          auto lhs_rhs = builder.context.get_operand(lhs_rhs_id);
          auto rhs_lhs = builder.context.get_operand(rhs_lhs_id);
          auto rhs_rhs = builder.context.get_operand(rhs_rhs_id);

          if (lhs_op == BinaryOp::Mul && rhs_op == BinaryOp::Mul && 
              lhs->use_id_list.size() == 1 && rhs->use_id_list.size() == 1) {
            if (lhs_lhs_id == rhs_lhs_id) {
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_rhs_id, rhs_rhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_lhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_lhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_rhs_id, rhs_rhs_id};

              changed = true;
            } else if (lhs_lhs_id == rhs_rhs_id) {
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_rhs_id, rhs_lhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_lhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_lhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_rhs_id, rhs_lhs_id};

              changed = true;
            } else if (lhs_rhs_id == rhs_lhs_id) {
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_lhs_id, rhs_rhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_rhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_rhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_lhs_id, rhs_rhs_id};

              changed = true;
            } else if (lhs_rhs_id == rhs_rhs_id) {
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_lhs_id, rhs_lhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_rhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_rhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_lhs_id, rhs_lhs_id};

              changed = true;
            }
          }
          if (lhs_op == BinaryOp::Mul) {
            if (rhs_id == lhs_lhs_id) {
              // (a * c) + a -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_rhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_rhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_lhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_lhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_rhs_id, constant_id};

              changed = true;
            } else if (rhs_id == lhs_rhs_id) {
              // (c * a) + a -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_lhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_lhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_rhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_rhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_lhs_id, constant_id};

              changed = true;
            }
          } else if (lhs_op == BinaryOp::Sub && lhs_lhs->is_zero()) {
            // (0 - x) + y -> y - x
            auto old_instr =
              builder.context.get_instruction(dst->maybe_def_id.value());

            auto sub_instr = builder.fetch_binary_instruction(
              BinaryOp::Sub, dst_id, rhs_id, lhs_rhs_id
            );

            old_instr->insert_next(sub_instr);

            old_instr->remove(builder.context);

            math_opt_ctx.expr_forest[dst_id] = {
              BinaryOp::Sub, rhs_id, lhs_rhs_id};

            changed = true;
          } else if (rhs_op == BinaryOp::Mul) {
            if (lhs_id == rhs_lhs_id) {
              // a + (a * c) -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(rhs_rhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, rhs_rhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, rhs_lhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, rhs_lhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, rhs_rhs_id, constant_id};

              changed = true;
            } else if (lhs_id == rhs_rhs_id) {
              // a + (c * a) -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(rhs_lhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, rhs_lhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, rhs_rhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, rhs_rhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, rhs_lhs_id, constant_id};

              changed = true;
            }
          } else if (rhs_op == BinaryOp::Sub && rhs_lhs->is_zero()) {
            // x + (0 - y) -> x - y
            auto old_instr =
              builder.context.get_instruction(dst->maybe_def_id.value());

            auto sub_instr = builder.fetch_binary_instruction(
              BinaryOp::Sub, dst_id, lhs_id, rhs_rhs_id
            );

            old_instr->insert_next(sub_instr);

            old_instr->remove(builder.context);

            math_opt_ctx.expr_forest[dst_id] = {
              BinaryOp::Sub, lhs_id, rhs_rhs_id};

            changed = true;
          }

        } else if (math_opt_ctx.expr_forest.count(lhs_id)) {
          auto [lhs_op, lhs_lhs_id, lhs_rhs_id] =
            math_opt_ctx.expr_forest.at(lhs_id);

          auto lhs_lhs = builder.context.get_operand(lhs_lhs_id);
          auto lhs_rhs = builder.context.get_operand(lhs_rhs_id);

          if (lhs_op == BinaryOp::Mul) {
            if (rhs_id == lhs_lhs_id) {
              // (a * c) + a -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_rhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_rhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_lhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_lhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_rhs_id, constant_id};

              changed = true;
            } else if (rhs_id == lhs_rhs_id) {
              // (c * a) + a -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_lhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, lhs_lhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_rhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_rhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, lhs_lhs_id, constant_id};

              changed = true;
            }
          } else if (lhs_op == BinaryOp::Sub && lhs_lhs->is_zero()) {
            // (0 - x) + y -> y - x
            auto old_instr =
              builder.context.get_instruction(dst->maybe_def_id.value());

            auto sub_instr = builder.fetch_binary_instruction(
              BinaryOp::Sub, dst_id, rhs_id, lhs_rhs_id
            );

            old_instr->insert_next(sub_instr);

            old_instr->remove(builder.context);

            math_opt_ctx.expr_forest[dst_id] = {
              BinaryOp::Sub, rhs_id, lhs_rhs_id};

            changed = true;
          }

        } else if (math_opt_ctx.expr_forest.count(rhs_id)) {
          auto [rhs_op, rhs_lhs_id, rhs_rhs_id] =
            math_opt_ctx.expr_forest.at(rhs_id);

          auto rhs_lhs = builder.context.get_operand(rhs_lhs_id);
          auto rhs_rhs = builder.context.get_operand(rhs_rhs_id);

          if (rhs_op == BinaryOp::Mul) {
            if (lhs_id == rhs_lhs_id) {
              // a + (a * c) -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(rhs_rhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, rhs_rhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, rhs_lhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, rhs_lhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, rhs_rhs_id, constant_id};

              changed = true;
            } else if (lhs_id == rhs_rhs_id) {
              // a + (c * a) -> a * (c + 1)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto constant_id = builder.fetch_constant_operand(
                builder.context.get_operand(rhs_lhs_id)->get_type(), 1
              );
              auto add_instr = builder.fetch_binary_instruction(
                BinaryOp::Add, tmp_id, rhs_lhs_id, constant_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, rhs_rhs_id, tmp_id
              );

              old_instr->insert_next(add_instr);
              add_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, rhs_rhs_id, tmp_id};
              math_opt_ctx.expr_forest[tmp_id] = {
                BinaryOp::Add, rhs_lhs_id, constant_id};

              changed = true;
            }
          } else if (rhs_op == BinaryOp::Sub && rhs_lhs->is_zero()) {
            // x + (0 - y) -> x - y
            auto old_instr =
              builder.context.get_instruction(dst->maybe_def_id.value());

            auto sub_instr = builder.fetch_binary_instruction(
              BinaryOp::Sub, dst_id, lhs_id, rhs_rhs_id
            );

            old_instr->insert_next(sub_instr);

            old_instr->remove(builder.context);

            math_opt_ctx.expr_forest[dst_id] = {
              BinaryOp::Sub, lhs_id, rhs_rhs_id};

            changed = true;
          }
        }
      } else if (math_opt_ctx.expr_forest.count(lhs_id)) {
        auto [lhs_op, lhs_lhs_id, lhs_rhs_id] =
          math_opt_ctx.expr_forest.at(lhs_id);

        auto lhs_lhs = builder.context.get_operand(lhs_lhs_id);
        auto lhs_rhs = builder.context.get_operand(lhs_rhs_id);

        if (lhs_op == BinaryOp::Add && lhs_rhs->is_constant() && rhs->is_constant()) {
          auto constant0 = std::get<operand::ConstantPtr>(lhs_rhs->kind);
          auto constant1 = std::get<operand::ConstantPtr>(rhs->kind);

          auto integer0 = std::get<int>(constant0->kind);
          auto integer1 = std::get<int>(constant1->kind);

          auto constant_id = builder.fetch_constant_operand(
            builder.context.get_operand(lhs_id)->get_type(), integer0 + integer1
          );

          auto old_instr =
            builder.context.get_instruction(dst->maybe_def_id.value());

          auto add_instr = builder.fetch_binary_instruction(
            BinaryOp::Add, dst_id, lhs_lhs_id, constant_id
          );

          old_instr->insert_next(add_instr);

          old_instr->remove(builder.context);

          math_opt_ctx.expr_forest[dst_id] = {
            BinaryOp::Add, lhs_lhs_id, constant_id};

          changed = true;
        }
      }
    } else if (op == BinaryOp::Sub) {
      if (lhs_id == rhs_id) {
        auto use_id_list_copy = dst->use_id_list;
        for (auto instr_id : use_id_list_copy) {
          auto instr = builder.context.get_instruction(instr_id);
          auto constant_id = builder.fetch_constant_operand(
            builder.context.get_operand(lhs_id)->get_type(), 0
          );
          instr->replace_operand(dst_id, constant_id, builder.context);
        }
        changed = true;
      } else if (lhs->is_arbitrary() && rhs->is_arbitrary()) {
        // Cases:
        // 1. (a * c) - (a * d) -> a * (c - d), maybe swaped
        // 2. x - (x + y) -> 0 - y
        // 3. (x + y) - x -> y
        if (math_opt_ctx.expr_forest.count(lhs_id) && math_opt_ctx.expr_forest.count(rhs_id)) {
          auto [lhs_op, lhs_lhs_id, lhs_rhs_id] =
            math_opt_ctx.expr_forest.at(lhs_id);
          auto [rhs_op, rhs_lhs_id, rhs_rhs_id] =
            math_opt_ctx.expr_forest.at(rhs_id);

          auto lhs_lhs = builder.context.get_operand(lhs_lhs_id);
          auto lhs_rhs = builder.context.get_operand(lhs_rhs_id);
          auto rhs_lhs = builder.context.get_operand(rhs_lhs_id);
          auto rhs_rhs = builder.context.get_operand(rhs_rhs_id);

          if (lhs_op == BinaryOp::Mul && rhs_op == BinaryOp::Mul && 
            lhs->use_id_list.size() == 1 && rhs->use_id_list.size() == 1) {
            if (lhs_lhs_id == rhs_lhs_id) {
              // (a * c) - (a * d) -> a * (c - d)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, tmp_id, lhs_rhs_id, rhs_rhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_lhs_id, tmp_id
              );

              old_instr->insert_next(sub_instr);
              sub_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_lhs_id, tmp_id};

              changed = true;
            } else if (lhs_lhs_id == rhs_rhs_id) {
              // (a * c) - (d * a) -> a * (c - d)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, tmp_id, lhs_rhs_id, rhs_lhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_lhs_id, tmp_id
              );

              old_instr->insert_next(sub_instr);
              sub_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_lhs_id, tmp_id};

              changed = true;
            } else if (lhs_rhs_id == rhs_lhs_id) {
              // (c * a) - (a * d) -> a * (c - d)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, tmp_id, lhs_lhs_id, rhs_rhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_rhs_id, tmp_id
              );

              old_instr->insert_next(sub_instr);
              sub_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_rhs_id, tmp_id};

              changed = true;
            } else if (lhs_rhs_id == rhs_rhs_id) {
              // (c * a) - (d * a) -> a * (c - d)
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto tmp_id = builder.fetch_arbitrary_operand(dst->get_type());
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, tmp_id, lhs_lhs_id, rhs_lhs_id
              );
              auto mul_instr = builder.fetch_binary_instruction(
                BinaryOp::Mul, dst_id, lhs_rhs_id, tmp_id
              );

              old_instr->insert_next(sub_instr);
              sub_instr->insert_next(mul_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Mul, lhs_rhs_id, tmp_id};

              changed = true;
            }
          } else if (lhs_op == BinaryOp::Add) {
            if (rhs_id == lhs_lhs_id) {
              // (x + y) - x -> y
              auto use_id_list_copy = dst->use_id_list;
              for (auto instr_id : use_id_list_copy) {
                auto instr = builder.context.get_instruction(instr_id);
                instr->replace_operand(dst_id, lhs_rhs_id, builder.context);
              }
              changed = true;
            } else if (rhs_id == lhs_rhs_id) {
              // (y + x) - x -> y
              auto use_id_list_copy = dst->use_id_list;
              for (auto instr_id : use_id_list_copy) {
                auto instr = builder.context.get_instruction(instr_id);
                instr->replace_operand(dst_id, lhs_lhs_id, builder.context);
              }
              changed = true;
            }
          } else if (rhs_op == BinaryOp::Add) {
            if (lhs_id == rhs_lhs_id) {
              // x - (x + y) -> 0 - y
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto zero_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_id)->get_type(), 0
              );
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, dst_id, zero_id, rhs_rhs_id
              );

              old_instr->insert_next(sub_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Sub, zero_id, rhs_rhs_id};

              changed = true;
            } else if (lhs_id == rhs_rhs_id) {
              // x - (y + x) -> 0 - y
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto zero_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_id)->get_type(), 0
              );
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, dst_id, zero_id, rhs_lhs_id
              );

              old_instr->insert_next(sub_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Sub, zero_id, rhs_lhs_id};

              changed = true;
            }
          }

        } else if (math_opt_ctx.expr_forest.count(lhs_id)) {
          auto [lhs_op, lhs_lhs_id, lhs_rhs_id] =
            math_opt_ctx.expr_forest.at(lhs_id);

          auto lhs_lhs = builder.context.get_operand(lhs_lhs_id);
          auto lhs_rhs = builder.context.get_operand(lhs_rhs_id);
          if (lhs_op == BinaryOp::Add) {
            if (rhs_id == lhs_lhs_id) {
              // (x + y) - x -> y
              auto use_id_list_copy = dst->use_id_list;
              for (auto instr_id : use_id_list_copy) {
                auto instr = builder.context.get_instruction(instr_id);
                instr->replace_operand(dst_id, lhs_rhs_id, builder.context);
              }
              changed = true;
            } else if (rhs_id == lhs_rhs_id) {
              // (y + x) - x -> y
              auto use_id_list_copy = dst->use_id_list;
              for (auto instr_id : use_id_list_copy) {
                auto instr = builder.context.get_instruction(instr_id);
                instr->replace_operand(dst_id, lhs_lhs_id, builder.context);
              }
              changed = true;
            }
          }

        } else if (math_opt_ctx.expr_forest.count(rhs_id)) {
          auto [rhs_op, rhs_lhs_id, rhs_rhs_id] =
            math_opt_ctx.expr_forest.at(rhs_id);

          auto rhs_lhs = builder.context.get_operand(rhs_lhs_id);
          auto rhs_rhs = builder.context.get_operand(rhs_rhs_id);
          if (rhs_op == BinaryOp::Add) {
            if (lhs_id == rhs_lhs_id) {
              // x - (x + y) -> 0 - y
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto zero_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_id)->get_type(), 0
              );
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, dst_id, zero_id, rhs_rhs_id
              );

              old_instr->insert_next(sub_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Sub, zero_id, rhs_rhs_id};

              changed = true;
            } else if (lhs_id == rhs_rhs_id) {
              // x - (y + x) -> 0 - y
              auto old_instr =
                builder.context.get_instruction(dst->maybe_def_id.value());
              auto zero_id = builder.fetch_constant_operand(
                builder.context.get_operand(lhs_id)->get_type(), 0
              );
              auto sub_instr = builder.fetch_binary_instruction(
                BinaryOp::Sub, dst_id, zero_id, rhs_lhs_id
              );

              old_instr->insert_next(sub_instr);

              old_instr->remove(builder.context);

              math_opt_ctx.expr_forest[dst_id] = {
                BinaryOp::Sub, zero_id, rhs_lhs_id};

              changed = true;
            }
          }
        }
      }
    } else if (op == BinaryOp::SDiv) {
      if (math_opt_ctx.expr_forest.count(lhs_id) 
      && rhs->is_constant() && rhs->is_int()) {
        // (x * c) / c -> x
        auto [lhs_op, lhs_lhs_id, lhs_rhs_id] =
          math_opt_ctx.expr_forest.at(lhs_id);
        
        auto lhs_lhs = builder.context.get_operand(lhs_lhs_id);
        auto lhs_rhs = builder.context.get_operand(lhs_rhs_id);
        if (lhs_op == BinaryOp::Mul) {
          if (lhs_rhs->is_constant() && lhs_rhs->is_int() && lhs->use_id_list.size() == 1) {
            auto rhs_constant = std::get<operand::ConstantPtr>(rhs->kind);
            auto rhs_constant_value = std::get<int>(rhs_constant->kind);
            auto lhs_rhs_constant = std::get<operand::ConstantPtr>(lhs_rhs->kind);
            auto lhs_rhs_constant_value = std::get<int>(lhs_rhs_constant->kind);
            if (rhs_constant_value == lhs_rhs_constant_value) {
              auto use_id_list_copy = dst->use_id_list;
              for (auto instr_id : use_id_list_copy) {
                auto instr = builder.context.get_instruction(instr_id);
                instr->replace_operand(dst_id, lhs_lhs_id, builder.context);
              }
              changed = true;
            }
          }
        }
      }
    }
  }
  return changed;
}

}  // namespace ir
}  // namespace syc