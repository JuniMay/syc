#include "passes__ir__cse.h"

namespace syc {
namespace ir {

void local_cse(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    local_cse_function(function, builder);
  }
}

void local_cse_function(FunctionPtr function, Builder& builder) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    local_cse_basic_block(curr_basic_block, builder);
    curr_basic_block = curr_basic_block->next;
  }
}

void local_cse_basic_block(BasicBlockPtr basic_block, Builder& builder) {
  using namespace instruction;

  std::map<
    std::tuple<BinaryOp, OperandID, std::variant<int, OperandID>>, OperandID>
    binary_expr_map;
  std::map<
    std::tuple<size_t, OperandID, std::vector<std::variant<int, OperandID>>>,
    OperandID>
    getelementptr_expr_map;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;

    auto maybe_binary = curr_instruction->as<Binary>();
    auto maybe_getelementptr = curr_instruction->as<GetElementPtr>();

    if (maybe_binary.has_value()) {
      auto binary = maybe_binary.value();
      std::variant<int, OperandID> rhs_id_copy;
      if (builder.context.get_operand(binary.rhs_id)->is_constant()
          && builder.context.get_operand(binary.rhs_id)->is_int()) {
        auto constant = std::get<operand::ConstantPtr>(
          builder.context.get_operand(binary.rhs_id)->kind
        );
        auto constant_value = std::get<int>(constant->kind);
        rhs_id_copy = constant_value;
      } else {
        rhs_id_copy = binary.rhs_id;
      }
      auto key = std::make_tuple(binary.op, binary.lhs_id, rhs_id_copy);
      if (binary_expr_map.count(key)) {
        auto existed_operand_id = binary_expr_map[key];
        auto dst = builder.context.get_operand(binary.dst_id);
        auto use_id_list_copy = dst->use_id_list;
        for (auto use_id : use_id_list_copy) {
          auto use_instruction = builder.context.get_instruction(use_id);
          use_instruction->replace_operand(
            binary.dst_id, existed_operand_id, builder.context
          );
        }
        curr_instruction->remove(builder.context);
      } else {
        binary_expr_map[key] = binary.dst_id;
      }
    } else if (maybe_getelementptr.has_value()) {
      auto getelementptr = maybe_getelementptr.value();
      std::vector<std::variant<int, OperandID>> index_id_list_copy;
      for (auto index_id : getelementptr.index_id_list) {
        auto index_operand = builder.context.get_operand(index_id);
        if (index_operand->is_constant() && index_operand->is_int()) {
          auto constant = std::get<operand::ConstantPtr>(index_operand->kind);
          auto constant_value = std::get<int>(constant->kind);
          index_id_list_copy.push_back(constant_value);
        } else {
          index_id_list_copy.push_back(index_id);
        }
      }
      size_t type_index = getelementptr.basis_type->kind.index();
      auto key =
        std::make_tuple(type_index, getelementptr.ptr_id, index_id_list_copy);
      if (getelementptr_expr_map.count(key)) {
        auto existed_operand_id = getelementptr_expr_map[key];
        auto dst = builder.context.get_operand(getelementptr.dst_id);
        auto use_id_list_copy = dst->use_id_list;
        for (auto use_id : use_id_list_copy) {
          auto use_instruction = builder.context.get_instruction(use_id);
          use_instruction->replace_operand(
            getelementptr.dst_id, existed_operand_id, builder.context
          );
        }
        curr_instruction->remove(builder.context);
        // std::cout << "remove" << std::endl;
      } else {
        getelementptr_expr_map[key] = getelementptr.dst_id;
      }
    }
    curr_instruction = next_instruction;
  }
}

}  // namespace ir
}  // namespace syc