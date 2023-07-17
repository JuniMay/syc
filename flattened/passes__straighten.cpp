#include "passes__straighten.h"

namespace syc {
namespace ir {

void straighten(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    straighten_function(function, builder);
  }
}

void straighten_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    auto next_basic_block = curr_basic_block->next;

    bool only_br = true;
    BasicBlockID br_block_id;

    auto curr_instruction = curr_basic_block->head_instruction->next;
    while (curr_instruction != curr_basic_block->tail_instruction) {
      if (!curr_instruction->is_br()) {
        only_br = false;
        break;
      } else {
        auto br = std::get<instruction::Br>(curr_instruction->kind);
        auto br_block = builder.context.get_basic_block(br.block_id);
        if (br_block->pred_list.size() != 1) {
          only_br = false;
          break;
        } else {
          br_block_id = br.block_id;
        }
      }
      curr_instruction = curr_instruction->next;
    }

    if (only_br) {
      // Delete the block which has exactly one succ (one uncondbr instruction)
      // and the destination block has exactly one pred (the block we are
      auto pred_list_copy = curr_basic_block->pred_list;
      auto use_id_list_copy = curr_basic_block->use_id_list;
      auto br_block = builder.context.get_basic_block(br_block_id);

      for (auto instruction_id : use_id_list_copy) {
        auto instruction = builder.context.get_instruction(instruction_id);
        auto parent_basic_block_id = instruction->parent_block_id;
        auto parent_basic_block =
          builder.context.get_basic_block(parent_basic_block_id);
        if (instruction->is_br()) {
          auto& br = std::get<instruction::Br>(instruction->kind);
          br.block_id = br_block_id;

          br_block->add_use(instruction_id);
          curr_basic_block->remove_use(instruction_id);
          parent_basic_block->remove_succ(curr_basic_block->id);
          parent_basic_block->add_succ(br_block_id);
          curr_basic_block->remove_pred(parent_basic_block->id);
          br_block->add_pred(parent_basic_block->id);
        } else if (std::holds_alternative<instruction::CondBr>(instruction->kind
                   )) {
          auto& condbr = std::get<instruction::CondBr>(instruction->kind);
          if (condbr.then_block_id == curr_basic_block->id) {
            condbr.then_block_id = br_block_id;
          } else {
            condbr.else_block_id = br_block_id;
          }

          br_block->add_use(instruction_id);
          curr_basic_block->remove_use(instruction_id);
          parent_basic_block->remove_succ(curr_basic_block->id);
          parent_basic_block->add_succ(br_block_id);
          curr_basic_block->remove_pred(parent_basic_block->id);
          br_block->add_pred(parent_basic_block->id);
        } else if (instruction->is_phi()) {
          auto& phi = std::get<instruction::Phi>(instruction->kind);

          OperandID incoming_operand_id;

          for (auto [operand_id, block_id] : phi.incoming_list) {
            if (block_id == curr_basic_block->id) {
              incoming_operand_id = operand_id;
              break;
            }
          }

          // Remove current block
          phi.incoming_list.erase(
            std::remove_if(
              phi.incoming_list.begin(), phi.incoming_list.end(),
              [curr_basic_block](auto& tuple) {
                return std::get<1>(tuple) == curr_basic_block->id;
              }
            ),
            phi.incoming_list.end()
          );

          // Add pred and incoming operand
          for (auto pred_id : pred_list_copy) {
            instruction->add_phi_operand(
              incoming_operand_id, pred_id, builder.context
            );
          }

          br_block->add_use(instruction_id);
          curr_basic_block->remove_use(instruction_id);
        }
      }
      curr_basic_block->remove(builder.context);
    }

    curr_basic_block = next_basic_block;
  }
}
}  // namespace ir
}  // namespace syc