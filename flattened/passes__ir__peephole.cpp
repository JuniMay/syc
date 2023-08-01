#include "passes__ir__peephole.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void peephole(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    peephole_function(function, builder);
  }
}

void peephole_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    peephole_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void peephole_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;
  builder.set_curr_basic_block(basic_block);

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    auto maybe_binary = curr_instruction->as<Binary>();
    auto maybe_getelementptr = curr_instruction->as<GetElementPtr>();
    auto maybe_cast = curr_instruction->as<Cast>();
    auto maybe_phi = curr_instruction->as<Phi>();
    auto maybe_store = curr_instruction->as<Store>();

    if (maybe_binary.has_value()) {
      auto curr_kind = maybe_binary.value();
      auto curr_op = curr_kind.op;
      auto curr_dst = builder.context.get_operand(curr_kind.dst_id);
      auto curr_lhs = builder.context.get_operand(curr_kind.lhs_id);
      auto curr_rhs = builder.context.get_operand(curr_kind.rhs_id);

      if (curr_op == BinaryOp::Sub
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        // dest = sub lhs, c
        // ->
        // dest = add lhs, -c
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        curr_instruction->insert_next(builder.fetch_binary_instruction(
          BinaryOp::Add, curr_dst->id, curr_lhs->id,
          builder.fetch_constant_operand(
            builder.fetch_i32_type(), (int)-constant_value
          )
        ));
        next_instruction = curr_instruction->next;
        curr_instruction->remove(builder.context);
      } else if (curr_op == BinaryOp::Add
        && curr_lhs->is_int()
        && curr_rhs->is_int()
        && curr_rhs->is_constant()) {
        // dest = add lhs, 0
        // ->
        // make all uses of dest to lhs
        auto constant = std::get<operand::ConstantPtr>(curr_rhs->kind);
        auto constant_value = std::get<int>(constant->kind);
        if (constant_value == 0) {
          auto use_id_list_copy = curr_dst->use_id_list;
          for (auto use_instruction_id : use_id_list_copy) {
            auto instruction =
              builder.context.get_instruction(use_instruction_id);
            instruction->replace_operand(
              curr_dst->id, curr_lhs->id, builder.context
            );
          }
          curr_instruction->remove(builder.context);
        }
      }

    } else if (maybe_getelementptr.has_value()) {
      auto curr_kind = maybe_getelementptr.value();
      auto curr_dst = builder.context.get_operand(curr_kind.dst_id);
      auto curr_ptr = builder.context.get_operand(curr_kind.ptr_id);
      bool replacable = true;
      for (auto index_operand_id : curr_kind.index_id_list) {
        auto index_operand = builder.context.get_operand(index_operand_id);
        if (!index_operand->is_constant()) {
          replacable = false;
        } else {
          auto constant = std::get<operand::ConstantPtr>(index_operand->kind);
          auto constant_value = std::get<int>(constant->kind);
          if (constant_value != 0) {
            replacable = false;
          }
        }
      }
      if (replacable) {
        auto bitcast_instruction = builder.fetch_cast_instruction(
          CastOp::BitCast, curr_dst->id, curr_ptr->id
        );
        curr_instruction->insert_next(bitcast_instruction);
        next_instruction = curr_instruction->next;
        curr_instruction->remove(builder.context);
      }
    } else if (maybe_cast.has_value()) {
      auto curr_kind = maybe_cast.value();
      auto curr_dst = builder.context.get_operand(curr_kind.dst_id);
      auto curr_src = builder.context.get_operand(curr_kind.src_id);
      auto curr_op = curr_kind.op;
      if (curr_op == CastOp::BitCast && *curr_dst->get_type() == *curr_src->get_type()) {
        // bitcast dst, src are same type
        // -> make all dst to src
        auto use_id_list_copy = curr_dst->use_id_list;
        for (auto use_instruction_id : use_id_list_copy) {
          auto instruction =
            builder.context.get_instruction(use_instruction_id);
          instruction->replace_operand(
            curr_dst->id, curr_src->id, builder.context
          );
        }
        curr_instruction->remove(builder.context);
      }
    } else if (maybe_phi.has_value()) {
      auto phi = maybe_phi.value();

      if (phi.incoming_list.size() == 1) {
        auto [operand_id, block_id] = phi.incoming_list[0];
        auto dst = builder.context.get_operand(phi.dst_id);
        auto use_id_list_copy = dst->use_id_list;
        for (auto instruction_id : use_id_list_copy) {
          auto instruction = builder.context.get_instruction(instruction_id);
          instruction->replace_operand(dst->id, operand_id, builder.context);
        }
        curr_instruction->remove(builder.context);
      }
    } else if (maybe_store.has_value()) {
      // Adjacent load and store from/to the same address
      auto store = maybe_store.value();
      if (next_instruction != basic_block->tail_instruction) {
        auto maybe_load = next_instruction->as<Load>();
        if (maybe_load.has_value()) {
          auto load = maybe_load.value();

          auto store_ptr_id = store.ptr_id;
          auto load_ptr_id = load.ptr_id;

          auto store_src_id = store.value_id;
          auto load_dst_id = load.dst_id;

          if (store_ptr_id == load_ptr_id) {
            auto load_dst = builder.context.get_operand(load_dst_id);
            auto use_id_list_copy = load_dst->use_id_list;
            for (auto instr_id : use_id_list_copy) {
              auto instr = builder.context.get_instruction(instr_id);
              instr->replace_operand(
                load_dst_id, store_src_id, builder.context
              );
            }
            // Remove the load instruction
            next_instruction->remove(builder.context);
            next_instruction = curr_instruction->next;
          }
        }
      }
    }

    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc