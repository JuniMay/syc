#include "passes/ir/loop_analysis.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void detect_natural_loop(
  FunctionPtr function,
  Builder& builder,
  LoopOptContext& loop_opt_ctx
) {
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

  // Find loop
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

    if (loop_opt_ctx.loop_info_map.count(header_id) == 0) {
      loop_opt_ctx.loop_info_map[header_id] = LoopInfo{
        header_id,
        std::move(loop_body_id_set),
        {},
      };
    } else {
      // Merge loop body
      for (auto bb_id : loop_body_id_set) {
        loop_opt_ctx.loop_info_map[header_id].body_id_set.insert(bb_id);
      }
    }

    // Update (initialize) exit set
    loop_opt_ctx.loop_info_map[header_id].exiting_id_set.clear();

    for (auto bb_id : loop_opt_ctx.loop_info_map[header_id].body_id_set) {
      auto bb = builder.context.get_basic_block(bb_id);

      bool is_exit = false;

      for (auto succ_id : bb->succ_list) {
        if (loop_opt_ctx.loop_info_map[header_id].body_id_set.count(succ_id) == 0) {
          is_exit = true;
          break;
        }
      }

      if (is_exit) {
        loop_opt_ctx.loop_info_map[header_id].exiting_id_set.insert(bb_id);
      }
    }
  }
}

}  // namespace ir
}  // namespace syc