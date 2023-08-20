#ifndef SYC_PASSES_ASM_GREEDY_ALLOCATION_H_
#define SYC_PASSES_ASM_GREEDY_ALLOCATION_H_

#include "backend/register.h"
#include "common.h"
#include "passes/asm/control_flow_analysis.h"
#include "passes/asm/liveness_analysis.h"

namespace syc {
namespace backend {

/// AllocID is used if the ranges of a operand are splited
using AllocID = size_t;

enum class AllocStage {
  New = 5,
  Assign = 4,
  Split = 3,
  Spill = 2,
  Memory = 1,
  Done = 0,
};

/// Range with corresponding alloc_id, this is used for set
struct AllocRange {
  Range range;
  AllocID alloc_id;

  AllocRange(Range range, AllocID alloc_id)
    : alloc_id(alloc_id), range(range) {}

  bool operator<(const AllocRange& other) const {
    if (range.st < other.range.st) {
      return true;
    } else if (range.st > other.range.st) {
      return false;
    } else {
      return range.ed < other.range.ed;
    }
  }
};

/// Prioritized alloc id used in priority queue
struct PrioritizedAlloc {
  /// AllocID
  AllocID alloc_id;
  /// Block count and instruction count
  std::tuple<size_t, InstrNum, AllocStage, bool> priority;

  PrioritizedAlloc(
    AllocID alloc_id,
    std::tuple<size_t, InstrNum, AllocStage, bool> priority
  )
    : alloc_id(alloc_id), priority(priority) {}

  bool operator<(const PrioritizedAlloc& other) const {
    auto [block_cnt, instr_cnt, stage, hinted] = priority;
    auto [other_block_cnt, other_instr_cnt, other_stage, other_hinted] =
      other.priority;

    if (stage < other_stage) {
      return true;
    } else if (stage > other_stage) {
      return false;
    } else if (block_cnt < other_block_cnt) {
      return true;
    } else if (block_cnt > other_block_cnt) {
      return false;
    } else if (instr_cnt < other_instr_cnt) {
      return true;
    } else if (instr_cnt > other_instr_cnt) {
      return false;
    } else {
      return !hinted && other_hinted;
    }
  }
};

/// Trying to replicate the algorithm used in LLVM
struct GreedyAllocationContext {
  /// The weight of each instruction in a basic block
  std::unordered_map<BasicBlockID, float> block_weight_map;
  /// Allocated registers, std::nullopt for spilled
  std::unordered_map<AllocID, Register> alloc_map;
  /// AllocID and corresponding operand, one-to-one
  std::unordered_map<AllocID, OperandID> alloc_operand_map;
  /// Operand and corresponding AllocID, one-to-many
  std::unordered_map<AllocID, std::vector<Range>> alloc_range_map;
  /// Allocated alloc_id to registers, this is used to check the availability of
  /// a register
  std::unordered_map<Register, std::set<AllocID>, RegisterHash> occupied_map;
  /// Allocated range
  std::unordered_map<Register, std::set<AllocRange>, RegisterHash>
    occupied_range_map;

  std::unordered_map<AllocID, AllocStage> alloc_stage_map;

  std::priority_queue<PrioritizedAlloc> alloc_pq;

  /// The offset in memory for spilled operands
  std::unordered_map<OperandID, size_t> operand_spill_map;

  /// Record the used temporary registers in a instruction
  std::unordered_map<InstructionID, std::set<Register>> used_tmpreg_map;

  std::unordered_map<OperandID, OperandID> coalesce_map;
  std::unordered_map<OperandID, Register> hint_map;

  AllocID next_alloc_id = 0;

  AllocID get_next_alloc_id() { return next_alloc_id++; }

  GreedyAllocationContext() = default;

  void try_allocate(AllocID, Builder&, LivenessAnalysisContext&);
  void try_split(AllocID, Builder&);
  void spill(AllocID, FunctionPtr, Builder&, LivenessAnalysisContext&);
  void modify_code(FunctionPtr, Builder&, LivenessAnalysisContext&);
};

void gen_alloc_hint(
  FunctionPtr function,
  Builder& builder,
  GreedyAllocationContext& ga_ctx
);

/// Calculate the weight of each basic block
/// The weight is initialized as 1.0.
/// If the block is inside a loop, the weight is multiplied by 2.0.
/// If there are multilayer loops, the weight is multiplied by 2.0 for each
void calc_block_weight(
  FunctionPtr function,
  Builder& builder,
  GreedyAllocationContext& ga_ctx
);

/// Calculate the weight of each range
float calc_range_weight(Range range, GreedyAllocationContext& ga_ctx);

/// Calculate the weight of each operand
float calc_spill_weight(AllocID alloc_id, GreedyAllocationContext& ga_ctx);

/// Calculate the priority in allocation
std::tuple<size_t, InstrNum, AllocStage, bool>
calc_alloc_priority(AllocID alloc_id, GreedyAllocationContext& ga_ctx);

void greedy_allocation(FunctionPtr function, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif