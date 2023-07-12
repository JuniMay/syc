#ifndef SYC_PASSES_MEM2REG_H_
#define SYC_PASSES_MEM2REG_H_

#include "common.h"
#include "ir__builder.h"
#include "ir__function.h"

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

struct Mem2regContext {
  /// Sets of variables
  std::set<OperandID> variable_set;
  /// Def basic blocks of given operand
  std::map<OperandID, std::set<BasicBlockID>> def_map;
  /// Sets of basic blocks that phi is added for given operand
  std::map<OperandID, std::set<BasicBlockID>> inserted_map;
  /// Record phi instruction and correscponding operand
  std::map<InstructionID, OperandID> phi_map;
  /// Sets of worklist
  std::map<OperandID, std::queue<BasicBlockID>> worklist_map;
  /// Rename stacks
  std::map<OperandID, std::stack<OperandID>> rename_stack_map;

  Mem2regContext() = default;
};

void mem2reg(Builder& builder);

void insert_phi(
  FunctionPtr function,
  Builder& builder,
  ControlFlowAnalysisContext& cfa_ctx,
  Mem2regContext& mem2reg_ctx
);

void rename(
  BasicBlockPtr basic_block,
  Builder& builder,
  ControlFlowAnalysisContext& cfa_ctx,
  Mem2regContext& mem2reg_ctx
);

void control_flow_analysis(
  FunctionPtr function,
  Context& context,
  ControlFlowAnalysisContext& cfa_ctx
);

}  // namespace ir
}  // namespace syc

#endif