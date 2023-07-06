#include "passes/linear_scan.h"

namespace syc {
namespace backend {

InstructionNumber LinearScanContext::get_next_instruction_number() {
  return this->next_instruction_number++;
}

bool LiveIntervalEndComparator::operator()(OperandID lhs, OperandID rhs) const {
  auto lhs_live_interval = this->live_interval_map.at(lhs);
  auto rhs_live_interval = this->live_interval_map.at(rhs);

  if (lhs_live_interval.ed == rhs_live_interval.ed) {
    return lhs < rhs;
  }

  return lhs_live_interval.ed > rhs_live_interval.ed;
}

void depth_first_numbering(
  BasicBlockPtr basic_block,
  Builder& builder,
  LinearScanContext& linear_scan_context
) {
  if (linear_scan_context.visited[basic_block->id]) {
    return;
  }

  linear_scan_context.visited[basic_block->id] = true;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto number = linear_scan_context.get_next_instruction_number();

    linear_scan_context.instruction_number_map[number] = curr_instruction->id;

    for (auto operand_id : curr_instruction->def_id_list) {
      auto operand = builder.context.get_operand(operand_id);
      if (operand->is_vreg()) {
        linear_scan_context.live_interval_list.push_back(operand_id);
        linear_scan_context.live_interval_map[operand_id] = {
          number,
          number,
          operand->is_float(),
        };
      }
    }

    for (auto operand_id : curr_instruction->use_id_list) {
      auto operand = builder.context.get_operand(operand_id);
      if (linear_scan_context.live_interval_map.find(operand_id) ==
          linear_scan_context.live_interval_map.end()) {
        // This shall not happen.
        linear_scan_context.live_interval_map[operand_id] = {
          number,
          number,
          operand->is_float(),
        };
      } else {
        auto curr_ed = linear_scan_context.live_interval_map[operand_id].ed;
        if (curr_ed < number) {
          linear_scan_context.live_interval_map[operand_id].ed = number;
        }
      }
    }

    curr_instruction = curr_instruction->next;
  }

  for (auto basic_block_id : basic_block->succ_list) {
    depth_first_numbering(
      builder.context.get_basic_block(basic_block_id), builder,
      linear_scan_context
    );
  }

  if (basic_block->next->next != nullptr) {
    depth_first_numbering(basic_block->next, builder, linear_scan_context);
  }
}

void live_interval_analysis(
  FunctionPtr function,
  Builder& builder,
  LinearScanContext& linear_scan_context
) {
  linear_scan_context.visited.clear();
  linear_scan_context.next_instruction_number = 0;
  linear_scan_context.instruction_number_map.clear();
  linear_scan_context.live_interval_map.clear();
  linear_scan_context.live_interval_list.clear();

  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    linear_scan_context.visited[curr_basic_block->id] = false;
    curr_basic_block = curr_basic_block->next;
  }

  depth_first_numbering(
    function->head_basic_block->next, builder, linear_scan_context
  );
}

Register map_general_register(int i) {
  GeneralRegister reg;
  if (i <= 1) {
    reg = (GeneralRegister)(i + 8);
  } else {
    reg = (GeneralRegister)(i + 16);
  }
  return Register{reg};
}

Register map_float_register(int i) {
  FloatRegister reg;
  if (i <= 1) {
    reg = (FloatRegister)(i + 8);
  } else {
    reg = (FloatRegister)(i + 16);
  }
  return Register{reg};
}

void linear_scan(
  FunctionPtr function,
  Builder& builder,
  LinearScanContext& linear_scan_context
) {
  live_interval_analysis(function, builder, linear_scan_context);

  linear_scan_context.allocated_general_register_map.clear();
  linear_scan_context.allocated_float_register_map.clear();

  linear_scan_context.active_list =
    std::set<OperandID, LiveIntervalEndComparator>(
      {}, LiveIntervalEndComparator{linear_scan_context.live_interval_map}
    );

  for (int i = 0; i < 12; i++) {
    linear_scan_context.available_general_register_set.insert(i);
    linear_scan_context.available_float_register_set.insert(i);
  }

  std::sort(
    linear_scan_context.live_interval_list.begin(),
    linear_scan_context.live_interval_list.end(),
    [&linear_scan_context](auto lhs, auto rhs) {
      return linear_scan_context.live_interval_map[lhs].st <
             linear_scan_context.live_interval_map[rhs].st;
    }
  );

  for (size_t i = 0; i < linear_scan_context.live_interval_list.size(); i++) {
    auto operand_id = linear_scan_context.live_interval_list[i];
    auto live_interval = linear_scan_context.live_interval_map[operand_id];

    // Expire old intervals
    while (!linear_scan_context.active_list.empty()) {
      // last element in active list
      auto top_operand_id = *linear_scan_context.active_list.rbegin();
      auto top_live_interval =
        linear_scan_context.live_interval_map[top_operand_id];
      if (top_live_interval.ed < live_interval.st) {
        linear_scan_context.active_list.erase(
          linear_scan_context.active_list.find(top_operand_id)
        );
        if (linear_scan_context.allocated_general_register_map.count(
              top_operand_id
            )) {
          linear_scan_context.available_general_register_set.insert(
            linear_scan_context.allocated_general_register_map[top_operand_id]
          );
        }
        if (linear_scan_context.allocated_float_register_map.count(
              top_operand_id
            )) {
          linear_scan_context.available_float_register_set.insert(
            linear_scan_context.allocated_float_register_map[top_operand_id]
          );
        }
      } else {
        break;
      }
    }

    if (linear_scan_context.allocated_general_register_map.count(operand_id)) {
      continue;
    }

    if (linear_scan_context.allocated_float_register_map.count(operand_id)) {
      continue;
    }

    if (live_interval.is_float) {
      if (linear_scan_context.available_float_register_set.empty()) {
        // Spill
        OperandID conflict_operand_id;
        for (auto id : linear_scan_context.active_list) {
          if (linear_scan_context.live_interval_map[id].is_float) {
            conflict_operand_id = id;
            break;
          }
        }

        auto conflict_live_interval =
          linear_scan_context.live_interval_map[conflict_operand_id];

        if (conflict_live_interval.ed > live_interval.ed) {
          spill(conflict_operand_id, function, builder, linear_scan_context);

          linear_scan_context.allocated_float_register_map[operand_id] =
            linear_scan_context
              .allocated_float_register_map[conflict_operand_id];

          linear_scan_context.allocated_float_register_map.erase(
            linear_scan_context.allocated_float_register_map.find(
              conflict_operand_id
            )
          );
          linear_scan_context.active_list.erase(conflict_operand_id);
          linear_scan_context.active_list.insert(operand_id);
        } else {
          spill(operand_id, function, builder, linear_scan_context);
        }

      } else {
        int reg = *linear_scan_context.available_float_register_set.begin();
        linear_scan_context.available_float_register_set.erase(reg);
        linear_scan_context.allocated_float_register_map[operand_id] = reg;
        linear_scan_context.active_list.insert(operand_id);
      }
    } else {
      if (linear_scan_context.available_general_register_set.empty()) {
        // Spill
        OperandID conflict_operand_id;
        for (auto id : linear_scan_context.active_list) {
          if (!linear_scan_context.live_interval_map[id].is_float) {
            conflict_operand_id = id;
            break;
          }
        }

        auto conflict_live_interval =
          linear_scan_context.live_interval_map[conflict_operand_id];

        if (conflict_live_interval.ed > live_interval.ed) {
          spill(conflict_operand_id, function, builder, linear_scan_context);

          linear_scan_context.allocated_general_register_map[operand_id] =
            linear_scan_context
              .allocated_general_register_map[conflict_operand_id];

          linear_scan_context.allocated_general_register_map.erase(
            linear_scan_context.allocated_general_register_map.find(
              conflict_operand_id
            )
          );
          linear_scan_context.active_list.erase(conflict_operand_id);
          linear_scan_context.active_list.insert(operand_id);
        } else {
          spill(operand_id, function, builder, linear_scan_context);
        }

      } else {
        int reg = *linear_scan_context.available_general_register_set.begin();
        linear_scan_context.available_general_register_set.erase(reg);
        linear_scan_context.allocated_general_register_map[operand_id] = reg;
        linear_scan_context.active_list.insert(operand_id);
      }
    }
  }

  for (auto [operand_id, reg] :
       linear_scan_context.allocated_general_register_map) {
    auto operand = builder.context.get_operand(operand_id);

    function->insert_saved_general_register(reg);

    auto reg_id = builder.fetch_register(map_general_register(reg));

    if (operand->maybe_def_id.has_value()) {
      auto def_instruction =
        builder.context.get_instruction(operand->maybe_def_id.value());

      def_instruction->replace_operand(operand_id, reg_id);

      builder.context.get_operand(reg_id)->set_def(def_instruction->id);
    }

    for (auto use_instruction_id : operand->use_id_list) {
      auto use_instruction =
        builder.context.get_instruction(use_instruction_id);

      use_instruction->replace_operand(operand_id, reg_id);

      builder.context.get_operand(reg_id)->add_use(use_instruction->id);
    }
  }

  for (auto [operand_id, reg] :
       linear_scan_context.allocated_float_register_map) {
    auto operand = builder.context.get_operand(operand_id);

    function->insert_saved_float_register(reg);

    auto reg_id = builder.fetch_register(map_float_register(reg));

    if (operand->maybe_def_id.has_value()) {
      auto def_instruction =
        builder.context.get_instruction(operand->maybe_def_id.value());

      def_instruction->replace_operand(operand_id, reg_id);

      builder.context.get_operand(reg_id)->set_def(def_instruction->id);
    }

    for (auto use_instruction_id : operand->use_id_list) {
      auto use_instruction =
        builder.context.get_instruction(use_instruction_id);

      use_instruction->replace_operand(operand_id, reg_id);

      builder.context.get_operand(reg_id)->add_use(use_instruction->id);
    }
  }
}

void spill(
  OperandID operand_id,
  FunctionPtr function,
  Builder& builder,
  LinearScanContext& linear_scan_context
) {
  auto operand = builder.context.get_operand(operand_id);
  auto live_interval = linear_scan_context.live_interval_map[operand_id];

  auto offset = function->stack_frame_size;
  // Treat all operands as 8 bytes.
  function->stack_frame_size += 8;

  if (operand->maybe_def_id.has_value()) {
    auto def_instruction =
      builder.context.get_instruction(operand->maybe_def_id.value());
    OperandID tmp_register_id;
    InstructionPtr store_instruction;

    auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

    if (live_interval.is_float) {
      tmp_register_id = builder.fetch_register(Register{FloatRegister::Ft0});
      store_instruction = builder.fetch_float_store_instruction(
        instruction::FloatStore::FSD, sp_id, tmp_register_id,
        builder.fetch_immediate(offset)
      );
    } else {
      tmp_register_id = builder.fetch_register(Register{GeneralRegister::T0});
      store_instruction = builder.fetch_store_instruction(
        instruction::Store::SD, sp_id, tmp_register_id,
        builder.fetch_immediate(offset)
      );
    }

    def_instruction->replace_operand(operand_id, tmp_register_id);
    def_instruction->insert_next(store_instruction);

    builder.context.get_operand(tmp_register_id)
      ->set_def(store_instruction->id);
  }

  for (auto use_instruction_id : operand->use_id_list) {
    auto use_instruction = builder.context.get_instruction(use_instruction_id);
    OperandID tmp_register_id;
    InstructionPtr load_instruction;

    auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

    if (live_interval.is_float) {
      tmp_register_id = builder.fetch_register(Register{FloatRegister::Ft0});
      load_instruction = builder.fetch_float_load_instruction(
        instruction::FloatLoad::FLD, sp_id, tmp_register_id,
        builder.fetch_immediate(offset)
      );
    } else {
      tmp_register_id = builder.fetch_register(Register{GeneralRegister::T0});
      load_instruction = builder.fetch_load_instruction(
        instruction::Load::LD, sp_id, tmp_register_id,
        builder.fetch_immediate(offset)
      );
    }

    use_instruction->replace_operand(operand_id, tmp_register_id);
    use_instruction->insert_prev(load_instruction);

    builder.context.get_operand(tmp_register_id)->add_use(load_instruction->id);
  }
}

}  // namespace backend

}  // namespace syc