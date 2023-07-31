#include "passes__asm__liveness_analysis.h"

namespace syc {
namespace backend {

std::string LivenessAnalysisContext::dump(Context& context) const {
  // Stringify live ranges, both ends inclusive
  std::string live_range_str;
  for (auto& [operand_id, live_range_list] : live_range_map) {
    live_range_str += context.get_operand(operand_id)->to_string() + ": ";
    for (auto& live_range : live_range_list) {
      live_range_str += "[" + std::to_string(live_range.st) + ", " +
                        std::to_string(live_range.ed) + "] ";
    }
    live_range_str += "\n";
  }

  return live_range_str;
}

void dfn(BasicBlockPtr bb, Builder& builder, LivenessAnalysisContext& la_ctx) {
  if (la_ctx.visited_map[bb->id]) {
    return;
  }

  la_ctx.visited_map[bb->id] = true;

  la_ctx.live_def_map[bb->id] = std::set<OperandID>();
  la_ctx.live_use_map[bb->id] = std::set<OperandID>();

  auto curr_instr = bb->head_instruction->next;
  while (curr_instr != bb->tail_instruction) {
    auto instr_num = la_ctx.get_next_instr_num();
    la_ctx.instr_num_map[curr_instr->id] = instr_num;
    la_ctx.instr_map[instr_num] = curr_instr->id;

    for (auto operand_id : curr_instr->use_id_list) {
      auto operand = builder.context.get_operand(operand_id);
      if ((operand->is_vreg() || operand->is_reg()) && 
          !la_ctx.live_def_map[bb->id].count(operand_id)) {
        la_ctx.live_use_map[bb->id].insert(operand_id);
      }
    }

    for (auto operand_id : curr_instr->def_id_list) {
      auto operand = builder.context.get_operand(operand_id);
      if ((operand->is_vreg() || operand->is_reg())) {
        la_ctx.live_def_map[bb->id].insert(operand_id);
      }
    }

    curr_instr = curr_instr->next;
  }

  InstrNum st = la_ctx.instr_num_map[bb->head_instruction->next->id];
  InstrNum ed = la_ctx.instr_num_map[bb->tail_instruction->prev.lock()->id];

  la_ctx.block_range_map[bb->id] = std::make_pair(st, ed);

  for (auto succ_id : bb->succ_list) {
    dfn(builder.context.get_basic_block(succ_id), builder, la_ctx);
  }
}

void liveness_analysis(
  FunctionPtr function,
  Builder& builder,
  LivenessAnalysisContext& la_ctx
) {
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    la_ctx.visited_map[curr_bb->id] = false;
    curr_bb = curr_bb->next;
  }

  la_ctx.instr_num_map.clear();
  la_ctx.instr_map.clear();
  la_ctx.next_instr_num = 0;

  dfn(function->head_basic_block->next, builder, la_ctx);

  std::vector<BasicBlockID> bb_reverse_order;
  auto exit_bb = function->tail_basic_block->prev.lock();

  std::queue<BasicBlockID> bb_queue;
  bb_queue.push(exit_bb->id);

  curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    la_ctx.visited_map[curr_bb->id] = false;
    curr_bb = curr_bb->next;
  }

  while (!bb_queue.empty()) {
    auto bb_id = bb_queue.front();
    bb_queue.pop();

    if (la_ctx.visited_map[bb_id]) {
      continue;
    }

    la_ctx.visited_map[bb_id] = true;
    bb_reverse_order.push_back(bb_id);

    auto bb = builder.context.get_basic_block(bb_id);
    for (auto pred_id : bb->pred_list) {
      bb_queue.push(pred_id);
    }
  }

  // Compute live-in and live-out
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto bb_id : bb_reverse_order) {
      auto bb = builder.context.get_basic_block(bb_id);
      auto live_out = std::set<OperandID>();
      auto live_in = std::set<OperandID>();

      for (auto succ_id : bb->succ_list) {
        if (!la_ctx.live_in_map.count(succ_id)) {
          la_ctx.live_in_map[succ_id] = std::set<OperandID>();
        }
        // Union
        for (auto operand_id : la_ctx.live_in_map[succ_id]) {
          live_out.insert(operand_id);
        }
      }

      // live-in = (live-out - live-def) + live-use
      for (auto operand_id : live_out) {
        if (!la_ctx.live_def_map[bb_id].count(operand_id)) {
          live_in.insert(operand_id);
        }
      }
      for (auto operand_id : la_ctx.live_use_map[bb_id]) {
        live_in.insert(operand_id);
      }

      // Update live-in and live-out
      if (!la_ctx.live_in_map.count(bb_id)) {
        la_ctx.live_in_map[bb_id] = std::set<OperandID>();
      }

      if (!la_ctx.live_out_map.count(bb_id)) {
        la_ctx.live_out_map[bb_id] = std::set<OperandID>();
      }

      if (live_in != la_ctx.live_in_map[bb_id]) {
        changed = true;
        la_ctx.live_in_map[bb_id] = live_in;
      }

      if (live_out != la_ctx.live_out_map[bb_id]) {
        changed = true;
        la_ctx.live_out_map[bb_id] = live_out;
      }
    }
  }

  // Compute live ranges
  curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    auto curr_instr = curr_bb->head_instruction->next;
    auto exit_instr = curr_bb->tail_instruction->prev.lock();

    auto entry_instr_num = la_ctx.instr_num_map[curr_instr->id];
    auto exit_instr_num = la_ctx.instr_num_map[exit_instr->id];

    std::unordered_map<OperandID, Range> live_range_buffer;

    for (auto operand_id : la_ctx.live_out_map[curr_bb->id]) {
      auto operand = builder.context.get_operand(operand_id);
      if (operand->is_vreg() || operand->is_reg()) {
        live_range_buffer[operand_id] =
          Range{entry_instr_num, exit_instr_num, 0, curr_bb->id};
      }
    }

    while (exit_instr != curr_bb->head_instruction) {
      auto curr_instr_num = la_ctx.instr_num_map[exit_instr->id];
      for (auto operand_id : exit_instr->def_id_list) {
        auto operand = builder.context.get_operand(operand_id);

        if (operand->is_vreg() || operand->is_reg()) {
          if (!live_range_buffer.count(operand_id)) {
            // This is a dead def
            // Add range { curr_instr_num, curr_instr_num } directly to live
            // range
            la_ctx.live_range_map[operand_id].push_back(Range{
              curr_instr_num, curr_instr_num, 1, curr_bb->id});
          } else {
            // Cut the range
            live_range_buffer[operand_id].st = curr_instr_num;
            live_range_buffer[operand_id].instr_cnt += 1;
            // Add the range to live range
            la_ctx.live_range_map[operand_id].push_back(
              live_range_buffer[operand_id]
            );
            // Remove the range from buffer
            live_range_buffer.erase(operand_id);
          }
        }
      }

      for (auto operand_id : exit_instr->use_id_list) {
        auto operand = builder.context.get_operand(operand_id);

        if (operand->is_vreg() || operand->is_reg()) {
          if (!live_range_buffer.count(operand_id)) {
            // This is a new use
            // Add range { entry_instr_num, curr_instr_num } to the buffer
            live_range_buffer[operand_id] =
              Range{entry_instr_num, curr_instr_num, 1, curr_bb->id};
          } else {
            // Increase the instruction count
            live_range_buffer[operand_id].instr_cnt += 1;
          }
        }
      }

      exit_instr = exit_instr->prev.lock();
    }

    // Add remaining ranges in buffer to live range
    for (auto [operand_id, range] : live_range_buffer) {
      la_ctx.live_range_map[operand_id].push_back(range);
    }

    curr_bb = curr_bb->next;
  }

  // Sort live ranges
  for (auto& [operand_id, live_range_list] : la_ctx.live_range_map) {
    std::sort(
      live_range_list.begin(), live_range_list.end(),
      [](const Range& lhs, const Range& rhs) { return lhs.st < rhs.st; }
    );
  }
}

}  // namespace backend
}  // namespace syc