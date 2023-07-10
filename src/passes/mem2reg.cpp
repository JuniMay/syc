#include "passes/mem2reg.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void mem2reg(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    auto cfa_ctx = ControlFlowAnalysisContext();
    auto mem2reg_ctx = Mem2regContext();
    control_flow_analysis(function, builder.context, cfa_ctx);

    // Prepare variable set
    auto entry_bb = function->head_basic_block->next;
    auto curr_instr = entry_bb->head_instruction->next;
    while (curr_instr != entry_bb->tail_instruction && curr_instr->is_alloca()
    ) {
      auto next_instr = curr_instr->next;

      auto alloca = std::get<instruction::Alloca>(curr_instr->kind);

      auto allocated_type = alloca.allocated_type;

      // Only convert integer and float
      if (std::holds_alternative<type::Integer>(*allocated_type) || std::holds_alternative<type::Float>(*allocated_type)) {
        // Add the memory operand to variable set
        mem2reg_ctx.variable_set.insert(alloca.dst_id);
        // Initialize the maps
        mem2reg_ctx.def_map[alloca.dst_id] = {};
        mem2reg_ctx.inserted_map[alloca.dst_id] = {};
        mem2reg_ctx.worklist_map[alloca.dst_id] = {};
        mem2reg_ctx.rename_stack_map[alloca.dst_id] = {};
        // Remove alloca instruction
        curr_instr->remove(builder.context);
      }

      curr_instr = next_instr;
    }
    // Insert phi instruction
    insert_phi(function, builder, cfa_ctx, mem2reg_ctx);
    // Rename variables
    rename(entry_bb, builder, cfa_ctx, mem2reg_ctx);
  }
}

void insert_phi(
  FunctionPtr function,
  Builder& builder,
  ControlFlowAnalysisContext& cfa_ctx,
  Mem2regContext& mem2reg_ctx
) {
  // Add all definition into corresponding worklist
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    auto curr_instr = curr_bb->head_instruction->next;

    while (curr_instr != curr_bb->tail_instruction) {
      if (curr_instr->is_store()) {
        auto store = std::get<instruction::Store>(curr_instr->kind);
        auto ptr_id = store.ptr_id;
        // For a to-be-converted variable, every block of its definition is only
        // added into the worklist once
        if (mem2reg_ctx.variable_set.count(ptr_id) && !mem2reg_ctx.def_map[ptr_id].count(curr_bb->id)) {
          mem2reg_ctx.def_map[ptr_id].insert(curr_bb->id);
          mem2reg_ctx.worklist_map[ptr_id].push(curr_bb->id);
        }
      }
      curr_instr = curr_instr->next;
    }
    curr_bb = curr_bb->next;
  }

  for (auto& [operand_id, worklist] : mem2reg_ctx.worklist_map) {
    std::cout << operand_id << std::endl;
    // Memory operand. The type is pointer.
    auto operand = builder.context.get_operand(operand_id);

    while (!worklist.empty()) {
      auto bb_id = worklist.front();
      worklist.pop();

      for (auto df_id : cfa_ctx.dominance_frontier_map[bb_id]) {
        if (mem2reg_ctx.inserted_map[operand_id].count(df_id)) {
          // if block is already inserted, just skip
          continue;
        }
        // insert phi to the start of dominance frontier block
        auto df_bb = builder.context.get_basic_block(df_id);
        builder.set_curr_basic_block(df_bb);
        auto dst_id = builder.fetch_arbitrary_operand(
          std::get<type::Pointer>(*operand->type).value_type
        );
        auto phi_instr = builder.fetch_phi_instruction(dst_id, {});
        builder.prepend_instruction_to_curr_basic_block(phi_instr);

        // Add the phi instruction to the map
        mem2reg_ctx.phi_map[phi_instr->id] = operand_id;

        // Mark as inserted
        mem2reg_ctx.inserted_map[operand_id].insert(df_id);

        if (!mem2reg_ctx.def_map[operand_id].count(df_id)) {
          mem2reg_ctx.worklist_map[operand_id].push(df_id);
        }
      }
    }
  }
}

void rename(
  BasicBlockPtr basic_block,
  Builder& builder,
  ControlFlowAnalysisContext& cfa_ctx,
  Mem2regContext& mem2reg_ctx
) {
  std::map<OperandID, size_t> def_cnt_map;

  auto curr_instr = basic_block->head_instruction->next;

  while (curr_instr != basic_block->tail_instruction) {
    auto next_instr = curr_instr->next;

    if (curr_instr->is_phi()) {
      auto phi = std::get<instruction::Phi>(curr_instr->kind);
      auto variable_id = mem2reg_ctx.phi_map[curr_instr->id];
      auto dst_id = phi.dst_id;

      def_cnt_map[variable_id] = 1;

      mem2reg_ctx.rename_stack_map[variable_id].push(dst_id);

    } else if (curr_instr->is_load()) {
      auto load = std::get<instruction::Load>(curr_instr->kind);

      if (mem2reg_ctx.variable_set.count(load.ptr_id)) {
        auto operand_id = mem2reg_ctx.rename_stack_map[load.ptr_id].top();

        auto load_dst = builder.context.get_operand(load.dst_id);

        auto use_id_list_copy = load_dst->use_id_list;

        for (auto instr_id : use_id_list_copy) {
          auto instr = builder.context.get_instruction(instr_id);
          instr->replace_operand(load.dst_id, operand_id, builder.context);
        }

        curr_instr->remove(builder.context);
      }

    } else if (curr_instr->is_store()) {
      auto store = std::get<instruction::Store>(curr_instr->kind);

      if (mem2reg_ctx.variable_set.count(store.ptr_id)) {
        // push the value to the stack
        mem2reg_ctx.rename_stack_map[store.ptr_id].push(store.value_id);

        if (!def_cnt_map.count(store.ptr_id)) {
          def_cnt_map[store.ptr_id] = 1;
        } else {
          def_cnt_map[store.ptr_id] += 1;
        }

        curr_instr->remove(builder.context);
      }
    } else if (curr_instr->is_br()) {
      break;
    }

    curr_instr = next_instr;
  }

  // Edit the phi instruction
  for (auto succ_id : basic_block->succ_list) {
    auto succ_bb = builder.context.get_basic_block(succ_id);
    auto curr_instr = succ_bb->head_instruction->next;

    while (curr_instr->is_phi()) {
      if (mem2reg_ctx.rename_stack_map[mem2reg_ctx.phi_map[curr_instr->id]].size() > 0) {
        curr_instr->add_phi_operand(
          mem2reg_ctx.rename_stack_map[mem2reg_ctx.phi_map[curr_instr->id]].top(
          ),
          basic_block->id
        );
      } else {
        auto phi = std::get<instruction::Phi>(curr_instr->kind);
        auto phi_type = builder.context.get_operand(phi.dst_id)->type;

        if (std::holds_alternative<type::Integer>(*phi_type)) {
          curr_instr->add_phi_operand(
            builder.fetch_constant_operand(phi_type, (int)0), basic_block->id
          );
        } else if (std::holds_alternative<type::Float>(*phi_type)) {
          curr_instr->add_phi_operand(
            builder.fetch_constant_operand(phi_type, (float)0), basic_block->id
          );
        }
      }
      curr_instr = curr_instr->next;
    }
  }

  // DFS
  for (auto child_id : cfa_ctx.dom_tree[basic_block->id]) {
    rename(
      builder.context.get_basic_block(child_id), builder, cfa_ctx, mem2reg_ctx
    );
  }

  // Restore the stack
  for (auto [operand_id, def_cnt] : def_cnt_map) {
    for (size_t i = 0; i < def_cnt; i++) {
      mem2reg_ctx.rename_stack_map[operand_id].pop();
    }
  }
}

void control_flow_analysis(
  FunctionPtr function,
  Context& context,
  ControlFlowAnalysisContext& cfa_ctx
) {
  cfa_ctx.dom_set_map.clear();
  cfa_ctx.idom_map.clear();
  cfa_ctx.height_map.clear();
  cfa_ctx.dominance_frontier_map.clear();

  // Compute dom
  auto entry_bb = function->head_basic_block->next;

  auto bb_id_set = std::set<BasicBlockID>();

  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    bb_id_set.insert(curr_bb->id);
    curr_bb = curr_bb->next;
  }

  curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    if (curr_bb == entry_bb) {
      cfa_ctx.dom_set_map[curr_bb->id] = {curr_bb->id};
    } else {
      cfa_ctx.dom_set_map[curr_bb->id] = bb_id_set;
    }
    cfa_ctx.dom_tree[curr_bb->id] = {};
    curr_bb = curr_bb->next;
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto bb_id : bb_id_set) {
      if (bb_id == entry_bb->id) {
        continue;
      }

      auto new_dom_set = bb_id_set;

      auto bb = context.get_basic_block(bb_id);

      for (auto pred_id : bb->pred_list) {
        // intersection
        std::set<BasicBlockID> intersection;
        std::set_intersection(
          new_dom_set.begin(), new_dom_set.end(),
          cfa_ctx.dom_set_map[pred_id].begin(),
          cfa_ctx.dom_set_map[pred_id].end(),
          std::inserter(intersection, intersection.begin())
        );
        new_dom_set = intersection;
      }

      new_dom_set.insert(bb_id);

      if (new_dom_set != cfa_ctx.dom_set_map[bb_id]) {
        changed = true;
        cfa_ctx.dom_set_map[bb_id] = new_dom_set;
      }
    }
  }

  // DEBUG
  std::cout << "dom" << std::endl;
  for (const auto& [bb_id, dom_set] : cfa_ctx.dom_set_map) {
    std::cout << "BB" << bb_id << ": ";
    for (auto dom_id : dom_set) {
      std::cout << "BB" << dom_id << " ";
    }
    std::cout << std::endl;
  }

  // Compute height using bfs
  std::queue<BasicBlockID> bb_queue;
  bb_queue.push(entry_bb->id);
  cfa_ctx.height_map[entry_bb->id] = 0;

  while (!bb_queue.empty()) {
    auto curr_bb_id = bb_queue.front();
    bb_queue.pop();

    auto curr_bb = context.get_basic_block(curr_bb_id);

    for (auto succ_id : curr_bb->succ_list) {
      if (cfa_ctx.height_map.find(succ_id) == cfa_ctx.height_map.end()) {
        cfa_ctx.height_map[succ_id] = cfa_ctx.height_map[curr_bb_id] + 1;
        bb_queue.push(succ_id);
      }
    }
  }

  // Compute idom
  for (const auto& [bb_id, dom_set] : cfa_ctx.dom_set_map) {
    if (dom_set.size() == 1) {
      // only entry_bb
      cfa_ctx.idom_map[bb_id] = std::nullopt;
      continue;
    }

    auto bb = context.get_basic_block(bb_id);

    if (bb->pred_list.size() == 1) {
      cfa_ctx.idom_map[bb_id] = bb->pred_list[0];
      continue;
    }

    // All dom satisfy that dom.height < bb.height
    // so the idom is the dom with the max height
    auto max_height = 0;
    BasicBlockID idom_id = entry_bb->id;

    for (auto dom_id : dom_set) {
      if (dom_id == bb_id) {
        continue;
      }

      if (cfa_ctx.height_map[dom_id] >= max_height) {
        max_height = cfa_ctx.height_map[dom_id];
        idom_id = dom_id;
      }
    }

    cfa_ctx.idom_map[bb_id] = idom_id;
  }

  // Construct the dom tree
  for (const auto& [bb_id, idom_id] : cfa_ctx.idom_map) {
    if (idom_id.has_value()) {
      cfa_ctx.dom_tree[idom_id.value()].push_back(bb_id);
    }
  }

  // DEBUG
  std::cout << "idom" << std::endl;
  for (const auto& [bb_id, idom_id] : cfa_ctx.idom_map) {
    std::cout << "BB" << bb_id << ": ";
    if (idom_id.has_value()) {
      std::cout << "BB" << idom_id.value();
    } else {
      std::cout << "None";
    }
    std::cout << std::endl;
  }

  // Compute dominance frontier
  for (auto bb_id : bb_id_set) {
    cfa_ctx.dominance_frontier_map[bb_id] = {};
  }

  for (auto bb_id : bb_id_set) {
    auto bb = context.get_basic_block(bb_id);
    if (bb->pred_list.size() <= 1) {
      continue;
    }
    if (!cfa_ctx.idom_map[bb_id].has_value()) {
      continue;
    }
    for (auto pred_id : bb->pred_list) {
      auto runner_id = pred_id;
      while (runner_id != cfa_ctx.idom_map[bb_id].value()) {
        cfa_ctx.dominance_frontier_map[runner_id].insert(bb_id);
        runner_id = cfa_ctx.idom_map[runner_id].value();
      }
    }
  }

  // DEBUG
  std::cout << "dominance frontier" << std::endl;
  for (const auto& [bb_id, df_set] : cfa_ctx.dominance_frontier_map) {
    std::cout << "BB" << bb_id << ": ";
    for (auto df_id : df_set) {
      std::cout << "BB" << df_id << " ";
    }
    std::cout << std::endl;
  }
}

}  // namespace ir
}  // namespace syc