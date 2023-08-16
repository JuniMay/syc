#include "passes__ir__loop_analysis.h"
#include "ir__basic_block.h"
#include "ir__instruction.h"
#include "ir__operand.h"

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
    std::optional<BasicBlockID> maybe_exit_id = std::nullopt;
    for (auto bb_id : loop_opt_ctx.loop_info_map[header_id].body_id_set) {
      auto bb = builder.context.get_basic_block(bb_id);

      bool is_exit = false;

      for (auto succ_id : bb->succ_list) {
        if (!loop_opt_ctx.loop_info_map[header_id].body_id_set.count(succ_id)) {
          loop_opt_ctx.loop_info_map[header_id].exit_id_set.insert(succ_id);
          is_exit = true;
        }
      }

      if (is_exit) {
        loop_opt_ctx.loop_info_map[header_id].exiting_id_set.insert(bb_id);
      }
    }
  }

  loop_opt_ctx.dom_map = std::move(dom_map);
}

void lcssa_transform(FunctionPtr function, Builder& builder) {
  using namespace instruction;

  auto loop_opt_ctx = LoopOptContext();
  detect_natural_loop(function, builder, loop_opt_ctx);

  // Sort loop by body size
  std::vector<LoopInfo> loop_info_list;
  for (const auto& [header_id, loop_info] : loop_opt_ctx.loop_info_map) {
    loop_info_list.push_back(loop_info);
  }

  std::sort(
    loop_info_list.begin(), loop_info_list.end(),
    [](const LoopInfo& a, const LoopInfo& b) {
      return a.body_id_set.size() < b.body_id_set.size();
    }
  );

  std::set<BasicBlockID> new_exit_bb_set;

  for (auto loop_info : loop_info_list) {
    if (loop_info.exiting_id_set.size() > 1 || loop_info.exit_id_set.size() > 1) {
      continue;
    }

    auto exit_bb =
      builder.context.get_basic_block(*loop_info.exit_id_set.begin());
    auto exiting_bb =
      builder.context.get_basic_block(*loop_info.exiting_id_set.begin());

    std::set<OperandID> unclosed_operand_id_set;

    for (auto& bb_id : loop_info.body_id_set) {
      auto bb = builder.context.get_basic_block(bb_id);
      for (auto instr = bb->head_instruction->next;
           instr != bb->tail_instruction; instr = instr->next) {
        if (instr->maybe_def_id.has_value()) {
          auto def_operand =
            builder.context.get_operand(instr->maybe_def_id.value());
          for (auto use_id : def_operand->use_id_list) {
            auto use_instr = builder.context.get_instruction(use_id);
            if (!loop_info.body_id_set.count(use_instr->parent_block_id) && 
                !new_exit_bb_set.count(use_instr->parent_block_id)) {
              unclosed_operand_id_set.insert(def_operand->id);
            }
          }
        }
      }
    }

    builder.switch_function(function->name);
    auto new_exit_bb = builder.fetch_basic_block();
    builder.set_curr_basic_block(new_exit_bb);

    for (auto operand_id : unclosed_operand_id_set) {
      auto operand = builder.context.get_operand(operand_id);
      auto phi_dst_id = builder.fetch_arbitrary_operand(operand->get_type());
      auto phi_instr = builder.fetch_phi_instruction(
        phi_dst_id, {std::make_pair(operand->id, exiting_bb->id)}
      );
      builder.append_instruction(phi_instr);

      auto use_id_list_copy = operand->use_id_list;
      for (auto use_id : use_id_list_copy) {
        auto use_instr = builder.context.get_instruction(use_id);
        if (!loop_info.body_id_set.count(use_instr->parent_block_id) && 
            use_instr->parent_block_id != new_exit_bb->id) {
          use_instr->replace_operand(operand->id, phi_dst_id, builder.context);
        }
      }
    }

    auto br_instr = builder.fetch_br_instruction(exit_bb->id);
    builder.append_instruction(br_instr);

    auto exiting_instr = exiting_bb->tail_instruction->prev.lock();

    auto maybe_condbr = exiting_instr->as<CondBr>();
    auto maybe_br = exiting_instr->as<Br>();

    if (maybe_condbr.has_value()) {
      auto& condbr = exiting_instr->as_ref<CondBr>().value().get();
      if (condbr.then_block_id == exit_bb->id) {
        condbr.then_block_id = new_exit_bb->id;
      } else {
        condbr.else_block_id = new_exit_bb->id;
      }
    } else {
      auto& br = exiting_instr->as_ref<Br>().value().get();
      br.block_id = new_exit_bb->id;
    }

    exiting_bb->remove_succ(exit_bb->id);
    exiting_bb->add_succ(new_exit_bb->id);
    exit_bb->remove_pred(exiting_bb->id);
    exit_bb->add_pred(new_exit_bb->id);
    new_exit_bb->add_pred(exiting_bb->id);

    exit_bb->remove_use(exiting_instr->id);
    new_exit_bb->add_use(exiting_instr->id);

    exiting_bb->insert_next(new_exit_bb);

    loop_opt_ctx.loop_info_map[loop_info.header_id].exit_id_set.clear();
    loop_opt_ctx.loop_info_map[loop_info.header_id].exit_id_set.insert(
      new_exit_bb->id
    );
    new_exit_bb_set.insert(new_exit_bb->id);
  }
}

}  // namespace ir
}  // namespace syc