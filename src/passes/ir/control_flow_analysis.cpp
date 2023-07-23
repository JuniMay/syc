#include "passes/ir/control_flow_analysis.h"

namespace syc {
namespace ir {

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