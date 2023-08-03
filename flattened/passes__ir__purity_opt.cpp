
#include "passes__ir__purity_opt.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void purity_opt(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    auto purity_ctx = PurityOptContext();
    purity_opt_function(function, builder, purity_ctx);
  }
}

bool is_pure(
  FunctionPtr function,
  Builder& builder,
  PurityOptContext& purity_ctx,
  size_t depth
) {
  // TODO: Mutual recursion

  using namespace instruction;

  if (purity_ctx.purity_result.count(function->name)) {
    return purity_ctx.purity_result[function->name];
  }

  if (function->is_declare) {
    purity_ctx.purity_result[function->name] = false;
    return false;
  }

  if (depth > 5) {
    return false;
  }

  for (auto bb = function->head_basic_block->next;
       bb != function->tail_basic_block; bb = bb->next) {
    for (auto instr = bb->head_instruction->next; instr != bb->tail_instruction;
         instr = instr->next) {
      auto maybe_call = instr->as<Call>();
      auto maybe_store = instr->as<Store>();

      if (maybe_call.has_value()) {
        auto callee = builder.context.get_function(maybe_call->function_name);
        if (callee->is_declare) {
          return false;
        }
        if (callee->name == function->name) {
          continue;
        }
        if (!is_pure(callee, builder, purity_ctx, depth + 1)) {
          purity_ctx.purity_result[function->name] = false;
          return false;
        }
      } else if (maybe_store.has_value()) {
        purity_ctx.purity_result[function->name] = false;
        return false;
      }
    }
  }

  purity_ctx.purity_result[function->name] = true;
  return true;
}

void purity_opt_function(
  FunctionPtr function,
  Builder& builder,
  PurityOptContext& purity_ctx
) {
  using namespace instruction;
  for (auto bb = function->head_basic_block->next;
       bb != function->tail_basic_block; bb = bb->next) {
    std::map<std::tuple<std::string, std::vector<OperandID>>, OperandID>
      call_cache;
    for (auto instr = bb->head_instruction->next; instr != bb->tail_instruction;
         instr = instr->next) {
      auto maybe_call = instr->as<Call>();
      if (maybe_call.has_value()) {
        auto callee = builder.context.get_function(maybe_call->function_name);
        if (!is_pure(callee, builder, purity_ctx)) {
          continue;
        }

        if (maybe_call.value().maybe_dst_id.has_value()) {
          auto dst =
            builder.context.get_operand(maybe_call.value().maybe_dst_id.value()
            );

          if (dst->use_id_list.size() == 0) {
            // Remove the call instruction
            auto prev = instr->prev.lock();
            instr->remove(builder.context);
            instr = prev;

            continue;
          }
        }

        auto key =
          std::make_tuple(maybe_call->function_name, maybe_call->arg_id_list);
        if (call_cache.count(key)) {
          if (maybe_call->maybe_dst_id.has_value()) {
            auto dst =
              builder.context.get_operand(maybe_call->maybe_dst_id.value());

            auto cached_dst = builder.context.get_operand(call_cache[key]);

            auto use_id_list_copy = dst->use_id_list;

            for (auto use_id : use_id_list_copy) {
              auto use_instr = builder.context.get_instruction(use_id);
              use_instr->replace_operand(
                dst->id, cached_dst->id, builder.context
              );
            }
          }

          auto prev = instr->prev.lock();
          instr->remove(builder.context);
          instr = prev;
        } else {
          if (maybe_call->maybe_dst_id.has_value()) {
            call_cache[key] = maybe_call->maybe_dst_id.value();
          } else {
            // Remove
            auto prev = instr->prev.lock();
            instr->remove(builder.context);
            instr = prev;
          }
        }
      }
    }
  }
}

}  // namespace ir
}  // namespace syc