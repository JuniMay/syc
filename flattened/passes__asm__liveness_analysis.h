#ifndef SYC_PASSES_ASM_LIVENESS_ANALYSIS_H_
#define SYC_PASSES_ASM_LIVENESS_ANALYSIS_H_

#include "backend__basic_block.h"
#include "backend__builder.h"
#include "backend__function.h"
#include "backend__instruction.h"
#include "backend__operand.h"
#include "common.h"

namespace syc {
namespace backend {

using InstrNum = size_t;

struct Range {
  InstrNum st;
  InstrNum ed;

  InstrNum instr_cnt = 0;
  BasicBlockID block_id;

  bool conflict(const Range& other) const {
    return (st >= other.st && st < other.ed) ||
           (ed > other.st && ed <= other.ed) ||
           (st <= other.st && ed >= other.ed);
  }
};

struct LivenessAnalysisContext {
  /// Instruction number map
  std::unordered_map<InstructionID, InstrNum> instr_num_map;
  /// Instruction map
  std::unordered_map<InstrNum, InstructionID> instr_map;

  /// Visited map
  std::unordered_map<BasicBlockID, bool> visited_map;

  /// Live-def map
  std::unordered_map<BasicBlockID, std::set<OperandID>> live_def_map;
  /// Live-use map
  std::unordered_map<BasicBlockID, std::set<OperandID>> live_use_map;
  /// Live-in map
  std::unordered_map<BasicBlockID, std::set<OperandID>> live_in_map;
  /// Live-out map
  std::unordered_map<BasicBlockID, std::set<OperandID>> live_out_map;

  /// Live ranges
  std::unordered_map<OperandID, std::vector<Range>> live_range_map;

  /// Ranges in block
  std::unordered_map<BasicBlockID, std::pair<InstrNum, InstrNum>>
    block_range_map;

  InstrNum next_instr_num = 0;

  InstrNum get_next_instr_num() { return next_instr_num++; }

  LivenessAnalysisContext() = default;

  std::string dump(Context& context) const;
};

/// Depth-first numbering
void dfn(BasicBlockPtr bb, Builder& builder, LivenessAnalysisContext& la_ctx);

void liveness_analysis(
  FunctionPtr function,
  Builder& builder,
  LivenessAnalysisContext& la_ctx
);

}  // namespace backend
}  // namespace syc

#endif