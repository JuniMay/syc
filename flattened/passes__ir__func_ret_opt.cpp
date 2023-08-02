#include "passes__ir__func_ret_opt.h"

namespace syc {
namespace ir {

void func_ret_opt(Builder& builder) {
  using namespace instruction;
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }

    if (!function->maybe_return_operand_id.has_value()) {
      continue;
    }

    if (function_name == "main") {
      continue;
    }

    bool used_return = false;

    for (auto instr_id : function->caller_id_list) {
      auto instr = builder.context.get_instruction(instr_id);
      auto call = instr->as<Call>().value();

      if (!call.maybe_dst_id.has_value()) {
        continue;
      } 
      
      auto dst = builder.context.get_operand(call.maybe_dst_id.value());

      if (dst->use_id_list.size() > 0) {
        used_return = true;
        break;
      }
    }

    if (used_return) {
      continue;
    }

    // Make return type void
    function->return_type = builder.fetch_void_type();
    auto exit_bb = function->tail_basic_block->prev.lock();

    // Modify related instructions
    for (auto instr_id : function->caller_id_list) {
      auto instr = builder.context.get_instruction(instr_id);
      auto& call = instr->as_ref<Call>().value().get();
      call.maybe_dst_id = std::nullopt;
      instr->maybe_def_id = std::nullopt;
    }

    // Modify return instruction
    builder.set_curr_basic_block(exit_bb);
    auto ret_instr = builder.fetch_ret_instruction();

    function->maybe_return_operand_id = std::nullopt;

    auto old_ret_instr = exit_bb->tail_instruction->prev.lock();

    old_ret_instr->insert_next(ret_instr);
    old_ret_instr->remove(builder.context);
  }
}

}  // namespace ir
}  // namespace syc