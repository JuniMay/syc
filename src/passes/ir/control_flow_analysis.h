#ifndef SYC_PASSES_IR_CONTROL_FLOW_ANALYSIS_H_
#define SYC_PASSES_IR_CONTROL_FLOW_ANALYSIS_H_


#include "common.h"
#include "ir/builder.h"
#include "ir/function.h"


namespace syc {
namespace ir {

struct ControlFlowAnalysisContext {
  /// Immediate dominator of basic blocks
  std::map<BasicBlockID, std::optional<BasicBlockID>> idom_map;
  /// Height(depth) of basic blocks
  std::map<BasicBlockID, size_t> height_map;
  /// Dominance frontier of basic blocks
  std::map<BasicBlockID, std::set<BasicBlockID>> dominance_frontier_map;
  /// Dominator tree
  std::map<BasicBlockID, std::vector<BasicBlockID>> dom_tree;

  ControlFlowAnalysisContext() = default;
};

void control_flow_analysis(
  FunctionPtr function,
  Context& context,
  ControlFlowAnalysisContext& cfa_ctx
);

}  // namespace ir
}  // namespace syc

#endif