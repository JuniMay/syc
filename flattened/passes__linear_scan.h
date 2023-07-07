#ifndef SYC_PASSES_LINEAR_SCAN_H_
#define SYC_PASSES_LINEAR_SCAN_H_

#include "backend__basic_block.h"
#include "backend__builder.h"
#include "backend__function.h"
#include "backend__instruction.h"
#include "backend__operand.h"
#include "backend__register.h"
#include "ir__codegen.h"
#include "common.h"

namespace syc {
namespace backend {

using InstructionNumber = size_t;

struct LiveInterval {
  InstructionNumber st;
  InstructionNumber ed;
  bool is_float = false;
};

struct LiveIntervalEndComparator {
  bool operator()(OperandID lhs, OperandID rhs) const;

  std::map<OperandID, LiveInterval> live_interval_map;
};

struct LinearScanContext {
  std::map<OperandID, LiveInterval> live_interval_map;

  /// Sorted by live interval start.
  std::vector<OperandID> live_interval_list;

  /// Allocated register map.
  /// This is not the final register allocation. Some might be spilled.
  std::map<OperandID, int> allocated_general_register_map;
  std::map<OperandID, int> allocated_float_register_map;

  /// Sorted by live interval end.
  std::set<OperandID, LiveIntervalEndComparator> active_list;

  std::map<InstructionNumber, InstructionID> instruction_number_map;

  std::map<BasicBlockID, bool> visited;

  /// 0 - 11 for s0 - s11
  std::set<int> available_general_register_set;
  /// 0 - 11 for fs0 - fs11
  std::set<int> available_float_register_set;

  InstructionNumber next_instruction_number = 0;

  LinearScanContext() = default;

  InstructionNumber get_next_instruction_number();
};

Register map_general_register(int i);

Register map_float_register(int i);

void depth_first_numbering(
  BasicBlockPtr basic_block,
  Builder& builder,
  LinearScanContext& linear_scan_context
);

void live_interval_analysis(
  FunctionPtr function,
  Builder& builder,
  LinearScanContext& linear_scan_context
);

void linear_scan(
  FunctionPtr function,
  Builder& builder,
  LinearScanContext& linear_scan_context
);

void spill(
  OperandID operand_id,
  FunctionPtr function,
  Builder& builder,
  LinearScanContext& linear_scan_context
);

}  // namespace backend

}  // namespace syc

#endif