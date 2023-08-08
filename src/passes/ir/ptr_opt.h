#ifndef SYC_PASSES_PTR_OPT_H_
#define SYC_PASSES_PTR_OPT_H_

#include "common.h"
#include "ir/builder.h"

namespace syc {
namespace ir {

struct PtrObj {
  OperandID base_ptr_id;
  std::optional<size_t> offset;
};

struct MemObj {
  OperandID base_ptr_id;
  std::vector<uint8_t> data;
};

struct PtrOptContext {
  std::unordered_set<OperandID> base_ptr_set;
  std::unordered_map<OperandID, PtrObj> ptr_obj_map;
  std::unordered_map<OperandID, MemObj> mem_obj_map;
  std::unordered_set<OperandID> uncertain_mem_obj_set;

  PtrOptContext() = default;
};

void fill_data(operand::ConstantPtr constant, std::vector<uint8_t>& data);

void ptr_opt(Builder& builder);

std::optional<PtrObj>
get_ptr(OperandID& operand_id, Builder& builder, PtrOptContext& ptr_opt_ctx);

}  // namespace ir
}  // namespace syc

#endif