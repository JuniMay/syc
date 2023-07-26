#ifndef SYC_PASSES_IR_LOOP_OPT_H_
#define SYC_PASSES_IR_LOOP_OPT_H_

#include "common.h"
#include "passes/ir/control_flow_analysis.h"

namespace syc {
namespace ir {

struct LoopInfo {
  BasicBlockID header_id;
  std::set<BasicBlockID> body_id_set;
  std::set<BasicBlockID> exit_id_set;

  bool operator<(const LoopInfo& other) const {
    return header_id < other.header_id;
  }
};

struct LoopOptContext {
  std::map<BasicBlockID, LoopInfo> loop_info_map;

  LoopOptContext() = default;
};

void loop_opt(Builder& builder);

void detect_natural_loop(
  FunctionPtr function,
  Builder& builder,
  LoopOptContext& loop_opt_ctx
);

void code_motion(Builder& builder, LoopOptContext& loop_opt_ctx);

}  // namespace ir
}  // namespace syc

#endif