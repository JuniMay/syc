#include "passes__linear_scan.h"

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

  linear_scan_context.live_def_map[basic_block->id] = {};
  linear_scan_context.live_use_map[basic_block->id] = {};

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto number = linear_scan_context.get_next_instruction_number();

    linear_scan_context.instruction_number_map[curr_instruction->id] = number;

    for (auto operand_id : curr_instruction->use_id_list) {
      // Roughly calculate the intervals
      auto operand = builder.context.get_operand(operand_id);
      if (operand->is_vreg()) {
        if (!linear_scan_context.live_interval_map.count(operand_id)) {
          linear_scan_context.live_interval_map[operand_id] = {
            number,
            number,
            operand->is_float(),
          };
        } else {
          linear_scan_context.live_interval_map[operand_id].st = std::min(
            linear_scan_context.live_interval_map[operand_id].st, number
          );
          linear_scan_context.live_interval_map[operand_id].ed = std::max(
            linear_scan_context.live_interval_map[operand_id].ed, number
          );
        }

        if (!linear_scan_context.live_def_map[basic_block->id].count(operand_id
            )) {
          linear_scan_context.live_use_map[basic_block->id].insert(operand_id);
        }
      }
    }

    for (auto operand_id : curr_instruction->def_id_list) {
      // Roughly calculate the intervals
      auto operand = builder.context.get_operand(operand_id);
      if (operand->is_vreg()) {
        if (!linear_scan_context.live_interval_map.count(operand_id)) {
          linear_scan_context.live_interval_map[operand_id] = {
            number,
            number,
            operand->is_float(),
          };
        } else {
          linear_scan_context.live_interval_map[operand_id].st = std::min(
            linear_scan_context.live_interval_map[operand_id].st, number
          );
          linear_scan_context.live_interval_map[operand_id].ed = std::max(
            linear_scan_context.live_interval_map[operand_id].ed, number
          );
        }
        linear_scan_context.live_def_map[basic_block->id].insert(operand_id);
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
}

void live_interval_analysis(
  FunctionPtr function,
  Builder& builder,
  LinearScanContext& linear_scan_context
) {
  auto curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    linear_scan_context.visited[curr_basic_block->id] = false;
    curr_basic_block = curr_basic_block->next;
  }

  depth_first_numbering(
    function->head_basic_block->next, builder, linear_scan_context
  );

  // Reverse order of basic blocks
  std::vector<BasicBlockID> basic_block_revlst;
  auto exit_basic_block = function->tail_basic_block->prev.lock();

  std::queue<BasicBlockID> basic_block_queue;
  basic_block_queue.push(exit_basic_block->id);

  curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    linear_scan_context.visited[curr_basic_block->id] = false;
    curr_basic_block = curr_basic_block->next;
  }

  while (!basic_block_queue.empty()) {
    auto basic_block_id = basic_block_queue.front();
    basic_block_queue.pop();

    basic_block_revlst.push_back(basic_block_id);

    auto basic_block = builder.context.get_basic_block(basic_block_id);

    for (auto pred_id : basic_block->pred_list) {
      if (!linear_scan_context.visited[pred_id]) {
        linear_scan_context.visited[pred_id] = true;
        basic_block_queue.push(pred_id);
      }
    }
  }

  /// Compute live-ins and live-outs
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto basic_block_id : basic_block_revlst) {
      auto basic_block = builder.context.get_basic_block(basic_block_id);
      std::set<OperandID> live_out = {};

      for (auto succ_id : basic_block->succ_list) {
        if (!linear_scan_context.live_in_map.count(succ_id)) {
          linear_scan_context.live_in_map[succ_id] = {};
        }
        std::set<OperandID> tmp;
        std::set_union(
          live_out.begin(), live_out.end(),
          linear_scan_context.live_in_map[succ_id].begin(),
          linear_scan_context.live_in_map[succ_id].end(),
          std::inserter(tmp, tmp.begin())
        );
        live_out = tmp;
      }

      std::set<OperandID> tmp;
      auto live_def = linear_scan_context.live_def_map[basic_block_id];
      auto live_use = linear_scan_context.live_use_map[basic_block_id];

      std::set_difference(
        live_out.begin(), live_out.end(), live_def.begin(), live_def.end(),
        std::inserter(tmp, tmp.begin())
      );
      std::set<OperandID> live_in;
      std::set_union(
        tmp.begin(), tmp.end(), live_use.begin(), live_use.end(),
        std::inserter(live_in, live_in.begin())
      );

      if (!linear_scan_context.live_in_map.count(basic_block_id)) {
        linear_scan_context.live_in_map[basic_block_id] = {};
      }

      if (!linear_scan_context.live_out_map.count(basic_block_id)) {
        linear_scan_context.live_out_map[basic_block_id] = {};
      }

      if (linear_scan_context.live_out_map[basic_block_id] != live_out) {
        linear_scan_context.live_out_map[basic_block_id] = live_out;
        changed = true;
      }

      if (linear_scan_context.live_in_map[basic_block_id] != live_in) {
        linear_scan_context.live_in_map[basic_block_id] = live_in;
        changed = true;
      }
    }
  }

  // Compute live intervals
  for (auto basic_block_id : basic_block_revlst) {
    auto basic_block = builder.context.get_basic_block(basic_block_id);

    auto live_in = linear_scan_context.live_in_map[basic_block_id];
    auto live_out = linear_scan_context.live_out_map[basic_block_id];

    auto curr_st =
      linear_scan_context
        .instruction_number_map[basic_block->head_instruction->next->id];

    auto curr_ed =
      linear_scan_context
        .instruction_number_map[basic_block->tail_instruction->prev.lock()->id];

    // Operands in live_in but not in live_out
    std::set<OperandID> live_in_only;
    // Operands in live_out but not in live_in
    std::set<OperandID> live_out_only;
    // Operands in both live_in and live_out
    std::set<OperandID> live_in_out;

    std::set_difference(
      live_in.begin(), live_in.end(), live_out.begin(), live_out.end(),
      std::inserter(live_in_only, live_in_only.begin())
    );

    std::set_difference(
      live_out.begin(), live_out.end(), live_in.begin(), live_in.end(),
      std::inserter(live_out_only, live_out_only.begin())
    );

    std::set_intersection(
      live_in.begin(), live_in.end(), live_out.begin(), live_out.end(),
      std::inserter(live_in_out, live_in_out.begin())
    );

    auto curr_instruction = basic_block->head_instruction->next;
    while (curr_instruction != basic_block->tail_instruction) {
      for (auto operand_id : live_in_only) {
        linear_scan_context.live_interval_map[operand_id].st = std::min(
          linear_scan_context.live_interval_map[operand_id].st, curr_st
        );
      }

      for (auto operand_id : live_out_only) {
        linear_scan_context.live_interval_map[operand_id].ed = std::max(
          linear_scan_context.live_interval_map[operand_id].ed, curr_ed
        );
      }

      for (auto operand_id : live_in_out) {
        linear_scan_context.live_interval_map[operand_id].st = std::min(
          linear_scan_context.live_interval_map[operand_id].st, curr_st
        );
        linear_scan_context.live_interval_map[operand_id].ed = std::max(
          linear_scan_context.live_interval_map[operand_id].ed, curr_ed
        );
      }

      curr_instruction = curr_instruction->next;
    }
  }

  // Push into live interval list
  for (auto [operand_id, live_interval] :
       linear_scan_context.live_interval_map) {
    linear_scan_context.live_interval_list.push_back(operand_id);
  }
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

  for (int i = 1; i < 12; i++) {
    linear_scan_context.available_general_register_set.insert(i);
  }

  for (int i = 0; i < 12; i++) {
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

    auto def_id_list_copy = operand->def_id_list;

    for (auto def_instruction_id : def_id_list_copy) {
      auto def_instruction =
        builder.context.get_instruction(def_instruction_id);

      def_instruction->replace_operand(operand_id, reg_id, builder.context);
    }

    auto use_id_list_copy = operand->use_id_list;

    for (auto use_instruction_id : use_id_list_copy) {
      auto use_instruction =
        builder.context.get_instruction(use_instruction_id);

      use_instruction->replace_operand(operand_id, reg_id, builder.context);
    }
  }

  for (auto [operand_id, reg] :
       linear_scan_context.allocated_float_register_map) {
    auto operand = builder.context.get_operand(operand_id);

    function->insert_saved_float_register(reg);

    auto reg_id = builder.fetch_register(map_float_register(reg));

    auto def_id_list_copy = operand->def_id_list;

    for (auto def_instruction_id : def_id_list_copy) {
      auto def_instruction =
        builder.context.get_instruction(def_instruction_id);

      def_instruction->replace_operand(operand_id, reg_id, builder.context);
    }

    auto use_id_list_copy = operand->use_id_list;

    for (auto use_instruction_id : use_id_list_copy) {
      auto use_instruction =
        builder.context.get_instruction(use_instruction_id);

      use_instruction->replace_operand(operand_id, reg_id, builder.context);
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

  auto def_id_list_copy = operand->def_id_list;

  for (auto def_instruction_id : def_id_list_copy) {
    auto def_instruction = builder.context.get_instruction(def_instruction_id);
    auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

    if (live_interval.is_float) {
      if (check_itype_immediate(offset)) {
        auto tmp_register_id =
          builder.fetch_register(Register{FloatRegister::Ft0});
        auto store_instruction = builder.fetch_float_store_instruction(
          instruction::FloatStore::FSD, sp_id, tmp_register_id,
          builder.fetch_immediate((int32_t)offset)
        );
        def_instruction->replace_operand(
          operand_id, tmp_register_id, builder.context
        );
        def_instruction->insert_next(store_instruction);
      } else {
        auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
        auto ftmp_id = builder.fetch_register(Register{FloatRegister::Ft0});

        auto li_instruction =
          builder.fetch_li_instruction(t0_id, builder.fetch_immediate(offset));
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::ADD, t0_id, sp_id, t0_id
        );
        auto fsd_instruction = builder.fetch_float_store_instruction(
          instruction::FloatStore::FSD, t0_id, ftmp_id,
          builder.fetch_immediate(0)
        );

        def_instruction->replace_operand(operand_id, ftmp_id, builder.context);

        def_instruction->insert_next(li_instruction);
        li_instruction->insert_next(add_instruction);
        add_instruction->insert_next(fsd_instruction);
      }
    } else {
      if (check_itype_immediate(offset)) {
        auto tmp_register_id =
          builder.fetch_register(Register{GeneralRegister::T0});
        auto store_instruction = builder.fetch_store_instruction(
          instruction::Store::SD, sp_id, tmp_register_id,
          builder.fetch_immediate((int32_t)offset)
        );
        def_instruction->replace_operand(
          operand_id, tmp_register_id, builder.context
        );
        def_instruction->insert_next(store_instruction);
      } else {
        auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
        auto tmp_id = builder.fetch_register(Register{GeneralRegister::T1});

        auto li_instruction =
          builder.fetch_li_instruction(t0_id, builder.fetch_immediate(offset));
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::ADD, t0_id, sp_id, t0_id
        );
        auto sd_instruction = builder.fetch_store_instruction(
          instruction::Store::SD, t0_id, tmp_id, builder.fetch_immediate(0)
        );

        def_instruction->replace_operand(operand_id, tmp_id, builder.context);

        def_instruction->insert_next(li_instruction);
        li_instruction->insert_next(add_instruction);
        add_instruction->insert_next(sd_instruction);
      }
    }
  }

  auto use_id_list_copy = operand->use_id_list;

  for (auto use_instruction_id : use_id_list_copy) {
    auto use_instruction = builder.context.get_instruction(use_instruction_id);

    auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

    if (live_interval.is_float) {
      size_t treg_id = 7;
      auto bitvec = std::bitset<19>();

      for (size_t i = 7; i < 19; i++) {
        if (!linear_scan_context.used_temporary_register_map.count(
              use_instruction_id
            )) {
          treg_id = i;
          break;
        } else if (!linear_scan_context
                      .used_temporary_register_map[use_instruction_id][i]) {
          treg_id = i;
          bitvec =
            linear_scan_context.used_temporary_register_map[use_instruction_id];
          break;
        }
      }

      bitvec[treg_id] = true;
      linear_scan_context.used_temporary_register_map[use_instruction_id] =
        bitvec;

      FloatRegister reg_kind;

      if (treg_id <= 14) {
        reg_kind = (FloatRegister)(treg_id - 7 + (int)FloatRegister::Ft0);
      } else {
        reg_kind = (FloatRegister)(treg_id - 15 + (int)FloatRegister::Ft8);
      }

      if (check_itype_immediate(offset)) {
        auto tmp_register_id = builder.fetch_register(Register{reg_kind});
        auto load_instruction = builder.fetch_float_load_instruction(
          instruction::FloatLoad::FLD, tmp_register_id, sp_id,
          builder.fetch_immediate((int32_t)offset)
        );
        use_instruction->replace_operand(
          operand_id, tmp_register_id, builder.context
        );
        use_instruction->insert_prev(load_instruction);
      } else {
        auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
        auto ftmp_id = builder.fetch_register(Register{reg_kind});
        linear_scan_context.used_temporary_register_map[use_instruction_id][0] =
          true;

        auto li_instruction =
          builder.fetch_li_instruction(t0_id, builder.fetch_immediate(offset));
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::ADD, t0_id, sp_id, t0_id
        );
        auto fld_instruction = builder.fetch_float_load_instruction(
          instruction::FloatLoad::FLD, ftmp_id, t0_id,
          builder.fetch_immediate(0)
        );

        use_instruction->replace_operand(operand_id, ftmp_id, builder.context);

        use_instruction->insert_prev(fld_instruction);
        fld_instruction->insert_prev(add_instruction);
        add_instruction->insert_prev(li_instruction);
      }
    } else {
      size_t treg_id = 0;
      auto bitvec = std::bitset<19>();

      for (size_t i = 0; i < 7; i++) {
        if (!linear_scan_context.used_temporary_register_map.count(
              use_instruction_id
            )) {
          treg_id = i;
          break;
        } else if (!linear_scan_context
                      .used_temporary_register_map[use_instruction_id][i]) {
          treg_id = i;
          bitvec =
            linear_scan_context.used_temporary_register_map[use_instruction_id];
          break;
        }
      }

      bitvec[treg_id] = true;
      linear_scan_context.used_temporary_register_map[use_instruction_id] =
        bitvec;

      GeneralRegister reg_kind;
      if (treg_id <= 2) {
        reg_kind = (GeneralRegister)(treg_id + (int)GeneralRegister::T0);
      } else {
        reg_kind = (GeneralRegister)(treg_id - 3 + (int)GeneralRegister::T3);
      }

      if (check_itype_immediate(offset)) {
        auto tmp_register_id = builder.fetch_register(Register{reg_kind});
        auto load_instruction = builder.fetch_load_instruction(
          instruction::Load::LD, tmp_register_id, sp_id,
          builder.fetch_immediate((int32_t)offset)
        );
        use_instruction->replace_operand(
          operand_id, tmp_register_id, builder.context
        );
        use_instruction->insert_prev(load_instruction);
      } else {
        auto tmp_id = builder.fetch_register(Register{reg_kind});
        linear_scan_context.used_temporary_register_map[use_instruction_id][0] =
          true;

        auto li_instruction =
          builder.fetch_li_instruction(tmp_id, builder.fetch_immediate(offset));
        auto add_instruction = builder.fetch_binary_instruction(
          instruction::Binary::ADD, tmp_id, sp_id, tmp_id
        );
        auto ld_instruction = builder.fetch_load_instruction(
          instruction::Load::LD, tmp_id, tmp_id, builder.fetch_immediate(0)
        );

        use_instruction->replace_operand(operand_id, tmp_id, builder.context);

        use_instruction->insert_prev(ld_instruction);
        ld_instruction->insert_prev(add_instruction);
        add_instruction->insert_prev(li_instruction);
      }
    }
  }
}

}  // namespace backend

}  // namespace syc