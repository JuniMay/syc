#ifndef SYC_PASSES_ASM_CONTROL_FLOW_ANALYSIS_H_
#define SYC_PASSES_ASM_CONTROL_FLOW_ANALYSIS_H_

#include "backend__builder.h"
#include "backend__function.h"
#include "common.h"

namespace syc {
namespace backend {

struct ControlFlowAnalysisContext {
  /// Immediate dominator of basic blocks
  std::unordered_map<BasicBlockID, std::optional<BasicBlockID>> idom_map;
  /// Dominance frontier of basic blocks
  std::unordered_map<BasicBlockID, std::set<BasicBlockID>>
    dominance_frontier_map;
  /// Dominator tree
  std::unordered_map<BasicBlockID, std::vector<BasicBlockID>> dom_tree;

  ControlFlowAnalysisContext() = default;
};

void control_flow_analysis(
  FunctionPtr function,
  Context& context,
  ControlFlowAnalysisContext& cfa_ctx
);

}  // namespace backend
}  // namespace syc

#endif