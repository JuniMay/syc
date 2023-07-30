#ifndef SYC_PASSES_ASM_GREEDY_ALLOCATION_H_
#define SYC_PASSES_ASM_GREEDY_ALLOCATION_H_

#include "common.h"
#include "passes/asm/control_flow_analysis.h"
#include "passes/asm/liveness_analysis.h"

namespace syc {
namespace backend {

/// AllocID is used if the ranges of a operand are splited
using AllocID = size_t;

#define PHYS_GENERAL_REG(X) (Register{GeneralRegister::X})
#define PHYS_FLOAT_REG(X) (Register{FloatRegister::X})

const std::set<Register> REG_FOR_ALLOCATION = {
  PHYS_GENERAL_REG(S1),  PHYS_GENERAL_REG(S2),  PHYS_GENERAL_REG(S3),
  PHYS_GENERAL_REG(S4),  PHYS_GENERAL_REG(S5),  PHYS_GENERAL_REG(S6),
  PHYS_GENERAL_REG(S7),  PHYS_GENERAL_REG(S8),  PHYS_GENERAL_REG(S9),
  PHYS_GENERAL_REG(S10), PHYS_GENERAL_REG(S11),

  PHYS_FLOAT_REG(Fs1),   PHYS_FLOAT_REG(Fs2),   PHYS_FLOAT_REG(Fs3),
  PHYS_FLOAT_REG(Fs4),   PHYS_FLOAT_REG(Fs5),   PHYS_FLOAT_REG(Fs6),
  PHYS_FLOAT_REG(Fs7),   PHYS_FLOAT_REG(Fs8),   PHYS_FLOAT_REG(Fs9),
  PHYS_FLOAT_REG(Fs10),  PHYS_FLOAT_REG(Fs11),
};

const std::vector<Register> REG_FOR_SPILL_GENERAL = {
  PHYS_GENERAL_REG(T0), PHYS_GENERAL_REG(T1), PHYS_GENERAL_REG(T2),
  PHYS_GENERAL_REG(T3), PHYS_GENERAL_REG(T4), PHYS_GENERAL_REG(T5),
  PHYS_GENERAL_REG(T6),
};

const std::vector<Register> REG_FOR_SPILL_FLOAT = {
  PHYS_FLOAT_REG(Ft0), PHYS_FLOAT_REG(Ft1),  PHYS_FLOAT_REG(Ft2),
  PHYS_FLOAT_REG(Ft3), PHYS_FLOAT_REG(Ft4),  PHYS_FLOAT_REG(Ft5),
  PHYS_FLOAT_REG(Ft6), PHYS_FLOAT_REG(Ft7),  PHYS_FLOAT_REG(Ft8),
  PHYS_FLOAT_REG(Ft9), PHYS_FLOAT_REG(Ft10), PHYS_FLOAT_REG(Ft11),
};

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
  std::tuple<size_t, InstrNum, AllocStage> priority;

  PrioritizedAlloc(
    AllocID alloc_id,
    std::tuple<size_t, InstrNum, AllocStage> priority
  )
    : alloc_id(alloc_id), priority(priority) {}

  bool operator<(const PrioritizedAlloc& other) const {
    auto [block_cnt, instr_cnt, stage] = priority;
    auto [other_block_cnt, other_instr_cnt, other_stage] = other.priority;

    if (stage < other_stage) {
      return true;
    } else if (stage > other_stage) {
      return false;
    } else if (block_cnt < other_block_cnt) {
      return true;
    } else if (block_cnt > other_block_cnt) {
      return false;
    } else {
      return instr_cnt < other_instr_cnt;
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

  AllocID next_alloc_id = 0;

  AllocID get_next_alloc_id() { return next_alloc_id++; }

  GreedyAllocationContext() = default;

  void try_allocate(AllocID, Builder&);
  void try_split(AllocID, Builder&);
  void spill(AllocID, FunctionPtr, Builder&, LivenessAnalysisContext&);
  void modify_code(FunctionPtr, Builder&, LivenessAnalysisContext&);
};

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
std::tuple<size_t, InstrNum, AllocStage>
calc_alloc_priority(AllocID alloc_id, GreedyAllocationContext& ga_ctx);

void greedy_allocation(FunctionPtr function, Builder& builder);

}  // namespace backend
}  // namespace syc

#endif