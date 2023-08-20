#include "passes/asm/greedy_allocation.h"
#include "ir/codegen.h"

namespace syc {
namespace backend {

void calc_block_weight(
  FunctionPtr function,
  Builder& builder,
  GreedyAllocationContext& ga_ctx
) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    ga_ctx.block_weight_map[curr_bb->id] = 1.0f;
    curr_bb = curr_bb->next;
  }

  auto cfa_ctx = ControlFlowAnalysisContext();
  control_flow_analysis(function, builder.context, cfa_ctx);

  auto func_entry_bb = function->head_basic_block->next;

  // Get doms from idom and dom_tree
  auto dom_map = std::map<BasicBlockID, std::set<BasicBlockID>>();
  // Layer traversal
  auto queue = std::queue<BasicBlockID>();
  queue.push(func_entry_bb->id);
  while (!queue.empty()) {
    auto bb_id = queue.front();
    queue.pop();

    auto dom_set = std::set<BasicBlockID>();

    if (cfa_ctx.idom_map[bb_id].has_value()) {
      dom_set.insert(cfa_ctx.idom_map[bb_id].value());
    }

    if (cfa_ctx.dom_tree.count(bb_id) != 0) {
      for (auto child_id : cfa_ctx.dom_tree[bb_id]) {
        queue.push(child_id);
      }
    }

    // Union idom's doms
    if (cfa_ctx.idom_map[bb_id].has_value()) {
      for (auto idom_dom_id : dom_map[cfa_ctx.idom_map[bb_id].value()]) {
        dom_set.insert(idom_dom_id);
      }
    }

    dom_map[bb_id] = std::move(dom_set);
  }

  // Find back edges
  auto back_edge_set = std::set<std::pair<BasicBlockID, BasicBlockID>>();

  for (const auto& [bb_id, dom_set] : dom_map) {
    for (auto dom_id : dom_set) {
      auto bb = builder.context.get_basic_block(bb_id);
      auto dom_bb = builder.context.get_basic_block(dom_id);

      bool is_dom_succ =
        std::find(bb->succ_list.begin(), bb->succ_list.end(), dom_id) !=
        bb->succ_list.end();

      if (is_dom_succ) {
        back_edge_set.insert(std::make_pair(bb_id, dom_id));
      }
    }
  }

  for (auto [bb_id, dom_id] : back_edge_set) {
    std::stack<BasicBlockID> stk;
    auto header_id = dom_id;
    std::set<BasicBlockID> loop_body_id_set;

    loop_body_id_set.insert(header_id);
    loop_body_id_set.insert(bb_id);

    stk.push(bb_id);

    while (!stk.empty()) {
      auto bb_id = stk.top();
      stk.pop();

      auto bb = builder.context.get_basic_block(bb_id);

      for (auto pred_id : bb->pred_list) {
        if (loop_body_id_set.count(pred_id) == 0) {
          loop_body_id_set.insert(pred_id);
          stk.push(pred_id);
        }
      }
    }

    // Calculate loop weight
    for (auto bb_id : loop_body_id_set) {
      ga_ctx.block_weight_map[bb_id] *= 4.0f;
    }
  }
}

float calc_range_weight(Range range, GreedyAllocationContext& ga_ctx) {
  return range.instr_cnt * ga_ctx.block_weight_map[range.block_id];
}

float calc_spill_weight(AllocID alloc_id, GreedyAllocationContext& ga_ctx) {
  float weight = 0.0f;
  for (auto range : ga_ctx.alloc_range_map[alloc_id]) {
    weight += calc_range_weight(range, ga_ctx);
  }
  auto operand_id = ga_ctx.alloc_operand_map.at(alloc_id);
  if (ga_ctx.coalesce_map.count(operand_id)) {
    weight *= 1.5f;
  } else if (ga_ctx.hint_map.count(operand_id)) {
    weight *= 2.0f;
  }
  return weight;
}

std::tuple<size_t, InstrNum, AllocStage, bool>
calc_alloc_priority(AllocID alloc_id, GreedyAllocationContext& ga_ctx) {
  size_t block_cnt = 0;
  InstrNum instr_cnt = 0;

  for (auto range : ga_ctx.alloc_range_map[alloc_id]) {
    block_cnt += 1;
    instr_cnt += range.instr_cnt;
  }

  auto operand_id = ga_ctx.alloc_operand_map.at(alloc_id);

  bool hinted =
    ga_ctx.coalesce_map.count(operand_id) || ga_ctx.hint_map.count(operand_id);

  return std::make_tuple(
    block_cnt, instr_cnt, ga_ctx.alloc_stage_map[alloc_id], hinted
  );
}

void GreedyAllocationContext::try_allocate(
  AllocID alloc_id,
  Builder& builder,
  LivenessAnalysisContext& la_ctx
) {
  auto operand = builder.context.get_operand(alloc_operand_map[alloc_id]);
  auto range_list = alloc_range_map[alloc_id];

  std::optional<Register> maybe_allocated_reg = std::nullopt;

  auto conflict_map =
    std::unordered_map<Register, std::set<AllocID>, RegisterHash>();

  auto hard_conflict_set = std::set<Register>();

  std::vector<Register> reg_alloc_seq = {};

  if (hint_map.count(operand->id)) {
    if (std::find(REG_ALLOC.begin(), REG_ALLOC.end(), hint_map[operand->id]) != 
        REG_ALLOC.end()) {
      reg_alloc_seq.push_back(hint_map[operand->id]);
    }
    for (auto reg : REG_ALLOC) {
      if (!(reg == hint_map[operand->id])) {
        reg_alloc_seq.push_back(reg);
      }
    }
  } else {
    reg_alloc_seq = REG_ALLOC;
  }

  for (auto reg : reg_alloc_seq) {
    if (reg.is_float() != operand->is_float()) {
      continue;
    }

    // Range list is sorted.
    auto alloc_range = AllocRange(range_list[0], alloc_id);

    bool is_conflict = false;

    conflict_map[reg] = std::set<AllocID>();

    // Check conflict if the register might be used somewhere.
    if (REG_ARGS.count(reg) || REG_TEMP.count(reg)) {
      auto reg_id = builder.fetch_register(reg);
      for (auto reg_range : la_ctx.live_range_map[reg_id]) {
        for (const auto& range : range_list) {
          if (range.conflict(reg_range)) {
            is_conflict = true;
            hard_conflict_set.insert(reg);
            break;
          }
        }
        if (is_conflict) {
          break;
        }
      }
    }

    if (is_conflict) {
      continue;
    }

    if (occupied_map[reg].empty()) {
      maybe_allocated_reg = reg;
      break;
    }
    auto iter = occupied_range_map[reg].lower_bound(alloc_range);
    if (iter != occupied_range_map[reg].begin()) {
      iter--;
    }
    // Check conflict and find all conflicts
    while (iter != occupied_range_map[reg].end()) {
      const auto& occupied_range = iter->range;
      for (const auto& range : range_list) {
        if (range.conflict(occupied_range)) {
          is_conflict = true;
          conflict_map[reg].insert(iter->alloc_id);
          break;
        }
      }
      iter++;
    }

    if (!is_conflict) {
      maybe_allocated_reg = reg;
      break;
    }
  }

  if (maybe_allocated_reg.has_value()) {
    auto allocated_reg = maybe_allocated_reg.value();
    // Update occupied_map
    occupied_map[allocated_reg].insert(alloc_id);
    // Update occupied_range_map
    for (auto range : range_list) {
      occupied_range_map[allocated_reg].insert(AllocRange(range, alloc_id));
    }
    // Update alloc_map
    alloc_map[alloc_id] = allocated_reg;
    // Update alloc_stage_map
    alloc_stage_map[alloc_id] = AllocStage::Assign;

    if (coalesce_map.count(operand->id)) {
      auto coalesce_target_id = coalesce_map[operand->id];
      auto coalesce_target = builder.context.get_operand(coalesce_target_id);
      hint_map[coalesce_target->id] = allocated_reg;
    }

  } else {
    // Try to evict
    auto min_weight = std::numeric_limits<float>::max();
    Register min_weight_reg;

    for (const auto& [reg, conflict_list] : conflict_map) {
      if (hard_conflict_set.count(reg)) {
        continue;
      }

      float weight = 0.0f;
      for (auto conflict_id : conflict_list) {
        weight += calc_spill_weight(conflict_id, *this);
      }

      if (weight < min_weight) {
        min_weight = weight;
        min_weight_reg = reg;
      }
    }

    auto alloc_weight = calc_spill_weight(alloc_id, *this);

    if (alloc_weight <= min_weight) {
      // Update stage
      alloc_stage_map[alloc_id] = AllocStage::Split;
      // Put back to alloc_pq with lower priority
      auto priority = calc_alloc_priority(alloc_id, *this);
      alloc_pq.push(PrioritizedAlloc(alloc_id, priority));

    } else {
      // Evict
      for (auto conflict_id : conflict_map[min_weight_reg]) {
        // Update occupied_map
        occupied_map[min_weight_reg].erase(conflict_id);
        // Update occupied_range_map
        auto iter = occupied_range_map[min_weight_reg].begin();
        while (iter != occupied_range_map[min_weight_reg].end()) {
          if (iter->alloc_id == conflict_id) {
            iter = occupied_range_map[min_weight_reg].erase(iter);
          } else {
            iter++;
          }
        }
        // Update alloc_map
        alloc_map.erase(conflict_id);
        // Put back to alloc_pq
        auto priority = calc_alloc_priority(conflict_id, *this);
        alloc_pq.push(PrioritizedAlloc(conflict_id, priority));
      }
      // Update occupied_map
      occupied_map[min_weight_reg].insert(alloc_id);
      // Update occupied_range_map
      for (auto range : range_list) {
        occupied_range_map[min_weight_reg].insert(AllocRange(range, alloc_id));
      }
      // Update alloc_map
      alloc_map[alloc_id] = min_weight_reg;
    }
  }
}

void GreedyAllocationContext::try_split(AllocID alloc_id, Builder& builder) {
  // Too complicated, just spill
  // Update stage
  alloc_stage_map[alloc_id] = AllocStage::Spill;
  // Put back to alloc_pq with lower priority
  auto priority = calc_alloc_priority(alloc_id, *this);
  alloc_pq.push(PrioritizedAlloc(alloc_id, priority));
}

void GreedyAllocationContext::spill(
  AllocID alloc_id,
  FunctionPtr function,
  Builder& builder,
  LivenessAnalysisContext& la_ctx
) {
  // TODO: Spill at a adequate position and put the spilled ranges back into the
  // alloc_pq

  using namespace instruction;

  auto instr_def_worklist = std::vector<InstructionPtr>();
  auto instr_use_worklist = std::vector<InstructionPtr>();

  auto operand = builder.context.get_operand(alloc_operand_map[alloc_id]);

  auto range_list = alloc_range_map[alloc_id];

  // Find all instructions that define or use the operand
  for (const auto& range : range_list) {
    for (auto instr_num = range.st; instr_num <= range.ed; instr_num++) {
      auto instr_id = la_ctx.instr_map[instr_num];
      auto instr = builder.context.get_instruction(instr_id);

      if (std::find(instr->def_id_list.begin(), instr->def_id_list.end(),
                    operand->id) != instr->def_id_list.end()) {
        instr_def_worklist.push_back(instr);
      }
      if (std::find(instr->use_id_list.begin(), instr->use_id_list.end(),
                    operand->id) != instr->use_id_list.end()) {
        instr_use_worklist.push_back(instr);
      }
    }
  }

  size_t offset;

  if (operand_spill_map.count(operand->id)) {
    offset = operand_spill_map[operand->id];
  } else {
    offset = function->stack_frame_size;
    function->stack_frame_size += 8;
    operand_spill_map[operand->id] = offset;
  }

  // Spill at def
  for (auto instr : instr_def_worklist) {
    auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

    if (operand->is_float()) {
      if (check_itype_immediate(offset)) {
        auto ft0_id = builder.fetch_register(Register{FloatRegister::Ft0});
        auto fsd_instr = builder.fetch_float_store_instruction(
          FloatStore::FSD, sp_id, ft0_id,
          builder.fetch_immediate((int32_t)offset)
        );
        instr->replace_def_operand(operand->id, ft0_id, builder.context);
        instr->insert_next(fsd_instr);
      } else {
        auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
        auto ft0_id = builder.fetch_register(Register{FloatRegister::Ft0});
        auto li_instr =
          builder.fetch_li_instruction(t0_id, builder.fetch_immediate(offset));
        auto add_instr =
          builder.fetch_binary_instruction(Binary::ADD, t0_id, sp_id, t0_id);
        auto fsd_instr = builder.fetch_float_store_instruction(
          FloatStore::FSD, t0_id, ft0_id, builder.fetch_immediate(0)
        );

        instr->replace_def_operand(operand->id, ft0_id, builder.context);
        instr->insert_next(li_instr);
        li_instr->insert_next(add_instr);
        add_instr->insert_next(fsd_instr);
      }
    } else {
      if (check_itype_immediate(offset)) {
        auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
        auto sd_instr = builder.fetch_store_instruction(
          Store::SD, sp_id, t0_id, builder.fetch_immediate((int32_t)offset)
        );
        instr->replace_def_operand(operand->id, t0_id, builder.context);
        instr->insert_next(sd_instr);
      } else {
        auto t0_id = builder.fetch_register(Register{GeneralRegister::T0});
        auto t1_id = builder.fetch_register(Register{GeneralRegister::T1});
        auto li_instr =
          builder.fetch_li_instruction(t0_id, builder.fetch_immediate(offset));
        auto add_instr =
          builder.fetch_binary_instruction(Binary::ADD, t0_id, sp_id, t0_id);
        auto sd_instr = builder.fetch_store_instruction(
          Store::SD, t0_id, t1_id, builder.fetch_immediate(0)
        );

        instr->replace_def_operand(operand->id, t1_id, builder.context);
        instr->insert_next(li_instr);
        li_instr->insert_next(add_instr);
        add_instr->insert_next(sd_instr);
      }
    }
  }

  // Spill at use
  for (auto instr : instr_use_worklist) {
    auto sp_id = builder.fetch_register(Register{GeneralRegister::Sp});

    if (!used_tmpreg_map.count(instr->id)) {
      used_tmpreg_map[instr->id] = std::set<Register>();
    }

    if (operand->is_float()) {
      auto ftmp_index = 0;
      while (ftmp_index < REG_SPILL_FLOAT.size()) {
        if (used_tmpreg_map[instr->id].count(REG_SPILL_FLOAT[ftmp_index])) {
          ftmp_index++;
        } else {
          break;
        }
      }
      auto ftmp_id = builder.fetch_register(REG_SPILL_FLOAT[ftmp_index]);
      used_tmpreg_map[instr->id].insert(REG_SPILL_FLOAT[ftmp_index]);

      if (check_itype_immediate(offset)) {
        auto fld_instr = builder.fetch_float_load_instruction(
          FloatLoad::FLD, ftmp_id, sp_id,
          builder.fetch_immediate((int32_t)offset)
        );
        instr->replace_use_operand(operand->id, ftmp_id, builder.context);
        instr->insert_prev(fld_instr);
      } else {
        auto tmp_index = 0;
        while (tmp_index < REG_SPILL_GENERAL.size()) {
          if (used_tmpreg_map[instr->id].count(REG_SPILL_GENERAL[tmp_index])) {
            tmp_index++;
          } else {
            break;
          }
        }
        auto tmp_id = builder.fetch_register(REG_SPILL_GENERAL[tmp_index]);
        used_tmpreg_map[instr->id].insert(REG_SPILL_GENERAL[tmp_index]);

        auto li_instr =
          builder.fetch_li_instruction(tmp_id, builder.fetch_immediate(offset));
        auto add_instr =
          builder.fetch_binary_instruction(Binary::ADD, tmp_id, sp_id, tmp_id);
        auto fld_instr = builder.fetch_float_load_instruction(
          FloatLoad::FLD, ftmp_id, tmp_id, builder.fetch_immediate(0)
        );

        instr->replace_use_operand(operand->id, ftmp_id, builder.context);
        instr->insert_prev(fld_instr);
        fld_instr->insert_prev(add_instr);
        add_instr->insert_prev(li_instr);
      }
    } else {
      auto tmp_index = 0;
      while (tmp_index < REG_SPILL_GENERAL.size()) {
        if (used_tmpreg_map[instr->id].count(REG_SPILL_GENERAL[tmp_index])) {
          tmp_index++;
        } else {
          break;
        }
      }
      auto tmp_id = builder.fetch_register(REG_SPILL_GENERAL[tmp_index]);
      used_tmpreg_map[instr->id].insert(REG_SPILL_GENERAL[tmp_index]);

      if (check_itype_immediate(offset)) {
        auto ld_instr = builder.fetch_load_instruction(
          Load::LD, tmp_id, sp_id, builder.fetch_immediate((int32_t)offset)
        );
        instr->replace_use_operand(operand->id, tmp_id, builder.context);
        instr->insert_prev(ld_instr);
      } else {
        auto li_instr =
          builder.fetch_li_instruction(tmp_id, builder.fetch_immediate(offset));
        auto add_instr =
          builder.fetch_binary_instruction(Binary::ADD, tmp_id, sp_id, tmp_id);
        auto ld_instr = builder.fetch_load_instruction(
          Load::LD, tmp_id, tmp_id, builder.fetch_immediate(0)
        );

        instr->replace_use_operand(operand->id, tmp_id, builder.context);
        instr->insert_prev(ld_instr);
        ld_instr->insert_prev(add_instr);
        add_instr->insert_prev(li_instr);
      }
    }
  }
}

void GreedyAllocationContext::modify_code(
  FunctionPtr function,
  Builder& builder,
  LivenessAnalysisContext& la_ctx
) {
  for (auto [alloc_id, reg] : alloc_map) {
    auto operand_id = alloc_operand_map[alloc_id];
    auto range_list = alloc_range_map[alloc_id];

    for (const auto& range : range_list) {
      for (auto instr_num = range.st; instr_num <= range.ed; instr_num++) {
        auto instr =
          builder.context.get_instruction(la_ctx.instr_map[instr_num]);
        auto reg_id = builder.fetch_register(reg);

        if (std::find(instr->def_id_list.begin(), instr->def_id_list.end(), 
                      operand_id) != instr->def_id_list.end()) {
          instr->replace_def_operand(operand_id, reg_id, builder.context);
        }

        if (std::find(instr->use_id_list.begin(), instr->use_id_list.end(), 
                      operand_id) != instr->use_id_list.end()) {
          instr->replace_use_operand(operand_id, reg_id, builder.context);
        }
      }
    }

    // Add to saved registers
    function->add_saved_register(reg);
  }
}

void greedy_allocation(FunctionPtr function, Builder& builder) {
  auto ga_ctx = GreedyAllocationContext();
  auto la_ctx = LivenessAnalysisContext();

  liveness_analysis(function, builder, la_ctx);
  gen_alloc_hint(function, builder, ga_ctx);

  // Clear alloc_map
  ga_ctx.alloc_map.clear();

  // Initialize alloc_ids
  for (auto [operand_id, live_range_list] : la_ctx.live_range_map) {
    auto operand = builder.context.get_operand(operand_id);

    if (operand->is_vreg()) {
      auto alloc_id = ga_ctx.get_next_alloc_id();

      ga_ctx.alloc_operand_map[alloc_id] = operand_id;
      ga_ctx.alloc_range_map[alloc_id] = live_range_list;
      ga_ctx.alloc_stage_map[alloc_id] = AllocStage::New;
    }
  }

  // Initialize occupied_map
  for (auto reg : REG_ALLOC) {
    ga_ctx.occupied_map[reg] = std::set<AllocID>();
    ga_ctx.occupied_range_map[reg] = std::set<AllocRange>();
  }

  // Initialize block_weight_map
  calc_block_weight(function, builder, ga_ctx);

  // Initialize the priority queue
  ga_ctx.alloc_pq = std::priority_queue<PrioritizedAlloc>();

  for (auto [alloc_id, _] : ga_ctx.alloc_range_map) {
    ga_ctx.alloc_pq.emplace(alloc_id, calc_alloc_priority(alloc_id, ga_ctx));
  }

  // Allocate
  while (!ga_ctx.alloc_pq.empty()) {
    auto prioritized_alloc = ga_ctx.alloc_pq.top();
    ga_ctx.alloc_pq.pop();

    auto alloc_id = prioritized_alloc.alloc_id;

    auto alloc_stage = ga_ctx.alloc_stage_map[alloc_id];

    switch (alloc_stage) {
      case AllocStage::New:
      case AllocStage::Assign: {
        ga_ctx.try_allocate(alloc_id, builder, la_ctx);
        break;
      }
      case AllocStage::Split: {
        ga_ctx.try_split(alloc_id, builder);
        break;
      }
      case AllocStage::Spill: {
        ga_ctx.spill(alloc_id, function, builder, la_ctx);
        break;
      }
      default: {
        // Do nothing
      }
    }
  }

  // Modify code
  ga_ctx.modify_code(function, builder, la_ctx);
}

void gen_alloc_hint(
  FunctionPtr function,
  Builder& builder,
  GreedyAllocationContext& ga_ctx
) {
  using namespace instruction;
  for (auto bb = function->head_basic_block->next;
       bb != function->tail_basic_block; bb = bb->next) {
    for (auto instr = bb->head_instruction->next; instr != bb->tail_instruction;
         instr = instr->next) {
      auto maybe_binary = instr->as<Binary>();
      auto maybe_binary_imm = instr->as<BinaryImm>();
      auto maybe_float_binary = instr->as<FloatBinary>();

      if (maybe_binary.has_value()) {
        auto binary = maybe_binary.value();

        auto op = binary.op;
        auto rd = builder.context.get_operand(binary.rd_id);
        auto rs1 = builder.context.get_operand(binary.rs1_id);
        auto rs2 = builder.context.get_operand(binary.rs2_id);

        if (binary.op == Binary::ADD || binary.op == Binary::ADDW) {
          if (rd->is_vreg() && rs1->is_vreg() && rs2->is_zero()) {
            ga_ctx.coalesce_map[rd->id] = rs1->id;
            ga_ctx.coalesce_map[rs1->id] = rd->id;
          } else if (rd->is_vreg() && rs1->is_zero() && rs2->is_vreg()) {
            ga_ctx.coalesce_map[rd->id] = rs2->id;
            ga_ctx.coalesce_map[rs2->id] = rd->id;
          } else if (rd->is_reg() && rs1->is_vreg() && rs2->is_zero()) {
            ga_ctx.hint_map[rs1->id] = std::get<Register>(rd->kind);
          } else if (rd->is_reg() && rs1->is_zero() && rs2->is_vreg()) {
            ga_ctx.hint_map[rs2->id] = std::get<Register>(rd->kind);
          } else if (rd->is_vreg() && rs1->is_reg() && rs2->is_zero()) {
            ga_ctx.hint_map[rd->id] = std::get<Register>(rs1->kind);
          } else if (rd->is_vreg() && rs1->is_zero() && rs2->is_reg()) {
            ga_ctx.hint_map[rd->id] = std::get<Register>(rs2->kind);
          }
        }
      } else if (maybe_binary_imm.has_value()) {
        auto binary_imm = maybe_binary_imm.value();

        auto op = binary_imm.op;
        auto rd = builder.context.get_operand(binary_imm.rd_id);
        auto rs = builder.context.get_operand(binary_imm.rs_id);
        auto imm = builder.context.get_operand(binary_imm.imm_id);

        if (op == BinaryImm::ADDI || BinaryImm::ADDIW) {
          if (rd->is_vreg() && rs->is_vreg() && imm->is_zero()) {
            ga_ctx.coalesce_map[rd->id] = rs->id;
            ga_ctx.coalesce_map[rs->id] = rd->id;
          } else if (rd->is_reg() && rs->is_vreg() && imm->is_zero()) {
            ga_ctx.hint_map[rs->id] = std::get<Register>(rd->kind);
          } else if (rd->is_vreg() && rs->is_reg() && imm->is_zero()) {
            ga_ctx.hint_map[rd->id] = std::get<Register>(rs->kind);
          }
        }
      } else if (maybe_float_binary.has_value()) {
        auto float_binary = maybe_float_binary.value();

        auto op = float_binary.op;
        auto rd = builder.context.get_operand(float_binary.rd_id);
        auto rs1 = builder.context.get_operand(float_binary.rs1_id);
        auto rs2 = builder.context.get_operand(float_binary.rs2_id);

        if (float_binary.op == FloatBinary::FSGNJ && rs1->id == rs2->id) {
          if (rd->is_vreg() && rs1->is_vreg()) {
            ga_ctx.coalesce_map[rd->id] = rs1->id;
            ga_ctx.coalesce_map[rs1->id] = rd->id;
          } else if (rd->is_reg() && rs1->is_vreg()) {
            ga_ctx.hint_map[rs1->id] = std::get<Register>(rd->kind);
          } else if (rd->is_vreg() && rs1->is_reg()) {
            ga_ctx.hint_map[rd->id] = std::get<Register>(rs1->kind);
          }
        }
      }
    }
  }
}

}  // namespace backend
}  // namespace syc