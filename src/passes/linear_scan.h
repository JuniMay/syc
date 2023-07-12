#ifndef SYC_PASSES_LINEAR_SCAN_H_
#define SYC_PASSES_LINEAR_SCAN_H_

#include "backend/basic_block.h"
#include "backend/builder.h"
#include "backend/function.h"
#include "backend/instruction.h"
#include "backend/operand.h"
#include "backend/register.h"
#include "ir/codegen.h"
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
  /// Instruction number map
  std::map<InstructionID, InstructionNumber> instruction_number_map;

  /// Live-def map
  std::map<BasicBlockID, std::set<OperandID>> live_def_map;
  /// Live-use map
  std::map<BasicBlockID, std::set<OperandID>> live_use_map;
  /// Live-in map
  std::map<BasicBlockID, std::set<OperandID>> live_in_map;
  /// Live-out map
  std::map<BasicBlockID, std::set<OperandID>> live_out_map;
  
  /// Live intervals
  std::map<OperandID, LiveInterval> live_interval_map;
  /// Sorted by live interval start.
  std::vector<OperandID> live_interval_list;

  /// Allocated register map.
  /// This is not the final register allocation. Some might be spilled.
  std::map<OperandID, int> allocated_general_register_map;
  std::map<OperandID, int> allocated_float_register_map;

  /// Sorted by live interval end.
  std::set<OperandID, LiveIntervalEndComparator> active_list;

  /// General purpose dfs
  std::map<BasicBlockID, bool> visited;

  /// Record the usage of temporary registers.
  /// 0~6 for t0~t6
  /// 7~18 for ft0~ft11
  std::map<InstructionID, std::bitset<19>> used_temporary_register_map; 

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