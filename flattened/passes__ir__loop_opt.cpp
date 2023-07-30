#include "passes__ir__loop_opt.h"
#include "ir__basic_block.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void loop_opt(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }

    builder.switch_function(function_name);

    auto loop_opt_ctx = LoopOptContext();

    detect_natural_loop(function, builder, loop_opt_ctx);

    code_motion(builder, loop_opt_ctx);
  }
}

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
    loop_opt_ctx.loop_info_map[header_id].exit_id_set.clear();

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
        loop_opt_ctx.loop_info_map[header_id].exit_id_set.insert(bb_id);
      }
    }
  }
}

void code_motion(Builder& builder, LoopOptContext& loop_opt_ctx) {
  std::map<BasicBlockID, BasicBlockID> preheader_map;

  // Code motion
  for (auto& [header_id, loop_info] : loop_opt_ctx.loop_info_map) {
    // Create preheader
    auto preheader = builder.fetch_basic_block();
    preheader_map[header_id] = preheader->id;

    builder.set_curr_basic_block(preheader);

    std::map<BasicBlockID, bool> visited;
    std::queue<BasicBlockID> queue;

    for (auto bb_id : loop_info.body_id_set) {
      visited[bb_id] = false;
    }

    queue.push(header_id);

    while (!queue.empty()) {
      auto bb_id = queue.front();
      queue.pop();

      if (visited[bb_id]) {
        continue;
      }

      visited[bb_id] = true;

      auto bb = builder.context.get_basic_block(bb_id);

      auto curr_instr = bb->head_instruction->next;
      while (curr_instr != bb->tail_instruction) {
        auto next_instr = curr_instr->next;

        bool maybe_motion = curr_instr->is_binary() ||
                            curr_instr->as<instruction::ICmp>().has_value() ||
                            curr_instr->as<instruction::FCmp>().has_value() ||
                            curr_instr->as<instruction::Cast>().has_value() ||
                            curr_instr->is_getelementptr();

        if (maybe_motion) {
          bool is_invariant = true;

          for (auto operand_id : curr_instr->use_id_list) {
            auto operand = builder.context.get_operand(operand_id);

            if (operand->maybe_def_id.has_value()) {
              auto def_instr =
                builder.context.get_instruction(operand->maybe_def_id.value());
              if (loop_info.body_id_set.count(def_instr->parent_block_id)) {
                is_invariant = false;
                break;
              }
            } else {
              continue;
            }
          }

          if (is_invariant) {
            curr_instr->raw_remove();
            builder.append_instruction(curr_instr);
          }
        }

        curr_instr = next_instr;
      }

      for (auto succ_id : bb->succ_list) {
        if (loop_info.body_id_set.count(succ_id)) {
          queue.push(succ_id);
        }
      }
    }
  }

  // Insert preheader
  for (auto [header_id, preheader_id] : preheader_map) {
    // Move phi instructions
    auto header = builder.context.get_basic_block(header_id);
    auto preheader = builder.context.get_basic_block(preheader_id);

    builder.set_curr_basic_block(preheader);

    // Modify phi instructions
    auto curr_instr = header->head_instruction->next;

    std::vector<InstructionPtr> moved_instr_list;
    std::vector<InstructionPtr> kept_instr_list;

    while (curr_instr != header->tail_instruction && curr_instr->is_phi()) {
      auto next_instr = curr_instr->next;

      auto phi = curr_instr->as<instruction::Phi>().value();

      std::vector<std::tuple<OperandID, BasicBlockID>> kept_incoming_list;
      std::vector<std::tuple<OperandID, BasicBlockID>> moved_incoming_list;

      for (auto [incoming_operand_id, incoming_bb_id] : phi.incoming_list) {
        if (loop_opt_ctx.loop_info_map[header_id].body_id_set.count(
              incoming_bb_id
            )) {
          kept_incoming_list.push_back(
            std::make_tuple(incoming_operand_id, incoming_bb_id)
          );
        } else {
          moved_incoming_list.push_back(
            std::make_tuple(incoming_operand_id, incoming_bb_id)
          );
        }
      }

      auto moved_phi_dst_id = builder.fetch_arbitrary_operand(
        builder.context.get_operand(phi.dst_id)->type
      );

      auto moved_phi_instr = builder.fetch_phi_instruction(
        moved_phi_dst_id, std::move(moved_incoming_list)
      );

      kept_incoming_list.push_back(
        std::make_tuple(moved_phi_dst_id, preheader_id)
      );

      auto kept_phi_instr = builder.fetch_phi_instruction(
        phi.dst_id, std::move(kept_incoming_list)
      );

      // Remove old phi
      curr_instr->remove(builder.context);

      kept_instr_list.push_back(kept_phi_instr);
      moved_instr_list.push_back(moved_phi_instr);

      curr_instr = next_instr;
    }

    builder.set_curr_basic_block(header);
    // reverse traversal
    for (auto it = kept_instr_list.rbegin(); it != kept_instr_list.rend();
         ++it) {
      builder.prepend_instruction_to_curr_basic_block(*it);
    }

    builder.set_curr_basic_block(preheader);
    for (auto it = moved_instr_list.rbegin(); it != moved_instr_list.rend();
         ++it) {
      builder.prepend_instruction_to_curr_basic_block(*it);
    }

    // Modify preds' succs and branches
    auto pred_list_copy = header->pred_list;

    for (auto pred_id : pred_list_copy) {
      if (loop_opt_ctx.loop_info_map[header_id].body_id_set.count(pred_id)) {
        continue;
      }

      auto pred_bb = builder.context.get_basic_block(pred_id);

      // Modify succs
      for (auto& succ_id : pred_bb->succ_list) {
        if (succ_id == header_id) {
          succ_id = preheader_id;
        }
      }

      // Modify branches
      auto terminator = pred_bb->tail_instruction->prev.lock();

      auto maybe_br = terminator->as_ref<instruction::Br>();
      auto maybe_condbr = terminator->as_ref<instruction::CondBr>();

      if (maybe_br.has_value()) {
        auto& br = maybe_br.value().get();
        br.block_id = preheader_id;
      } else if (maybe_condbr.has_value()) {
        auto& condbr = maybe_condbr.value().get();
        if (condbr.then_block_id == header_id) {
          condbr.then_block_id = preheader_id;
        } else if (condbr.else_block_id == header_id) {
          condbr.else_block_id = preheader_id;
        }
      }

      header->remove_use(terminator->id);
      header->remove_pred(pred_id);
      preheader->add_use(terminator->id);
      preheader->add_pred(pred_id);
      pred_bb->remove_succ(header_id);
      pred_bb->add_succ(preheader_id);
    }
    // Insert br
    auto br_instr = builder.fetch_br_instruction(header_id);
    builder.append_instruction(br_instr);

    header->insert_prev(preheader);
  }
}

}  // namespace ir
}  // namespace syc