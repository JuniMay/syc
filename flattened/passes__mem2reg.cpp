#include "passes__mem2reg.h"
#include "ir__basic_block.h"
#include "ir__context.h"
#include "ir__instruction.h"
#include "ir__operand.h"

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

      bool convert = std::holds_alternative<type::Integer>(*allocated_type) ||
                     std::holds_alternative<type::Float>(*allocated_type) ||
                     std::holds_alternative<type::Pointer>(*allocated_type);

      // Only convert integer and float
      if (convert) {
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
          basic_block->id, builder.context
        );
      } else {
        auto phi = std::get<instruction::Phi>(curr_instr->kind);
        auto phi_type = builder.context.get_operand(phi.dst_id)->type;

        if (std::holds_alternative<type::Integer>(*phi_type)) {
          curr_instr->add_phi_operand(
            builder.fetch_constant_operand(phi_type, (int)0), basic_block->id,
            builder.context
          );
        } else if (std::holds_alternative<type::Float>(*phi_type)) {
          curr_instr->add_phi_operand(
            builder.fetch_constant_operand(phi_type, (float)0), basic_block->id,
            builder.context
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
  cfa_ctx.idom_map.clear();
  cfa_ctx.height_map.clear();
  cfa_ctx.dominance_frontier_map.clear();

  // Compute idom
  auto entry_bb = function->head_basic_block->next;

  auto bb_id_set = std::set<BasicBlockID>();
  auto bb_id_postorder = std::vector<BasicBlockID>();
  auto bb_id_visited = std::map<BasicBlockID, bool>();

  size_t postorder_number = 0;
  auto postorder_number_map = std::map<BasicBlockID, size_t>();

  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    bb_id_set.insert(curr_bb->id);
    bb_id_visited[curr_bb->id] = false;
    curr_bb = curr_bb->next;
  }

  // Get postorder
  std::function<void(BasicBlockID)> dfs = [&](BasicBlockID bb_id) {
    bb_id_visited[bb_id] = true;

    auto bb = context.get_basic_block(bb_id);

    for (auto succ_id : bb->succ_list) {
      if (!bb_id_visited[succ_id]) {
        dfs(succ_id);
      }
    }

    bb_id_postorder.push_back(bb_id);
    postorder_number_map[bb_id] = postorder_number;
    postorder_number++;
  };

  dfs(entry_bb->id);

  // Reverse postorder
  auto bb_id_rev_postorder = bb_id_postorder;
  std::reverse(bb_id_rev_postorder.begin(), bb_id_rev_postorder.end());

  // Initialize idom_map
  for (auto bb_id : bb_id_set) {
    cfa_ctx.idom_map[bb_id] = std::nullopt;
  }

  // Initialize entry block
  cfa_ctx.idom_map[entry_bb->id] = entry_bb->id;

  std::function<BasicBlockID(BasicBlockID, BasicBlockID)> intersect =
    [&](BasicBlockID bb_lhs, BasicBlockID bb_rhs) {
      auto finger1 = bb_lhs;
      auto finger2 = bb_rhs;

      while (finger1 != finger2) {
        while (postorder_number_map[finger1] < postorder_number_map[finger2]) {
          finger1 = cfa_ctx.idom_map[finger1].value();
        }
        while (postorder_number_map[finger2] < postorder_number_map[finger1]) {
          finger2 = cfa_ctx.idom_map[finger2].value();
        }
      }

      return finger1;
    };

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto bb_id : bb_id_rev_postorder) {
      if (bb_id == entry_bb->id) {
        continue;
      }

      auto bb = context.get_basic_block(bb_id);

      // Get first processed as new_idom (not nullopt)
      std::optional<BasicBlockID> new_idom = std::nullopt;
      for (auto pred_id : bb->pred_list) {
        if (cfa_ctx.idom_map[pred_id].has_value()) {
          new_idom = pred_id;
          break;
        }
      }

      if (!new_idom.has_value()) {
        continue;
      }

      for (auto pred_id : bb->pred_list) {
        if (cfa_ctx.idom_map[pred_id].has_value()) {
          new_idom = intersect(new_idom.value(), pred_id);
        }
      }

      if (cfa_ctx.idom_map[bb_id] != new_idom) {
        cfa_ctx.idom_map[bb_id] = new_idom;
        changed = true;
      }
    }
  }

  // Reset entry block
  cfa_ctx.idom_map[entry_bb->id] = std::nullopt;

  // Construct the dom tree
  for (const auto& [bb_id, idom_id] : cfa_ctx.idom_map) {
    if (idom_id.has_value()) {
      cfa_ctx.dom_tree[idom_id.value()].push_back(bb_id);
    }
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
}

}  // namespace ir
}  // namespace syc