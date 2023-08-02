#include "passes__ir__loop_indvar_simplify.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"
#include "passes__adt.h"

namespace syc {
namespace ir {

void loop_indvar_simplify(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    loop_indvar_simplify_function(function, builder);
  }
}

void loop_indvar_simplify_function(FunctionPtr function, Builder& builder) {
  auto loop_opt_ctx = LoopOptContext();
  detect_natural_loop(function, builder, loop_opt_ctx);

  for (auto [_, loop_info] : loop_opt_ctx.loop_info_map) {
    loop_indvar_simplify_helper(loop_info, builder);
  }
}

void loop_indvar_simplify_helper(LoopInfo& loop_info, Builder& builder) {
  using namespace instruction;

  // Check if there are no branches in the loop
  for (auto body_id : loop_info.body_id_set) {
    auto bb = builder.context.get_basic_block(body_id);
    if (bb->succ_list.size() > 1) {
      // If more than one succ point to the bb inside the loop, then there is
      // a branch inside the loop
      // Check how many succs are inside the loop body.
      size_t inside_succ_cnt = 0;
      for (auto succ_id : bb->succ_list) {
        if (loop_info.body_id_set.count(succ_id)) {
          inside_succ_cnt++;
        }
      }
      if (inside_succ_cnt > 1) {
        return;
      }
    }
  }

  DisjointSet<OperandID> ivset;
  std::unordered_map<OperandID, IvRecord> ivrecord_map;

  // Only find simple indvars
  // 1. Appear in the phi instruction of the loop header, with one value coming
  // from inside the loop and the other outside.
  // 2. The value coming from inside the loop is defined by add instruction.

  // Find all phi instructions in the loop header
  auto header_bb = builder.context.get_basic_block(loop_info.header_id);
  for (auto instr = header_bb->head_instruction->next;
       instr != header_bb->tail_instruction && instr->is_phi();
       instr = instr->next) {
    auto phi = instr->as<Phi>().value();
    if (phi.incoming_list.size() != 2) {
      continue;
    }

    std::optional<OperandID> maybe_alternate_id;
    std::optional<OperandID> maybe_start_id;
    std::optional<OperandID> maybe_step_id;

    for (auto [operand_id, block_id] : phi.incoming_list) {
      if (loop_info.body_id_set.count(block_id)) {
        maybe_alternate_id = operand_id;
      } else {
        maybe_start_id = operand_id;
      }
    }

    if (!maybe_alternate_id || !maybe_start_id) {
      continue;
    }

    auto alternative_id = maybe_alternate_id.value();
    auto start_id = maybe_start_id.value();

    ivset.make_set(phi.dst_id);
    ivset.make_set(alternative_id);
    ivset.union_set(phi.dst_id, alternative_id);

    auto alternative_operand = builder.context.get_operand(alternative_id);

    if (!alternative_operand->maybe_def_id.has_value()) {
      continue;
    }

    auto def_instr =
      builder.context.get_instruction(alternative_operand->maybe_def_id.value()
      );

    auto maybe_binary = def_instr->as<Binary>();

    if (maybe_binary.has_value()) {
      auto binary = maybe_binary.value();

      if (binary.op == BinaryOp::Add) {
        auto dst_id = binary.dst_id;
        auto lhs_id = binary.lhs_id;
        auto rhs_id = binary.rhs_id;

        auto rhs = builder.context.get_operand(rhs_id);

        if (lhs_id == phi.dst_id && dst_id == alternative_id && rhs->is_constant()) {
          auto representative = ivset.find_set(phi.dst_id).value();
          ivrecord_map[representative] =
            IvRecord{phi.dst_id, start_id, binary.op, rhs_id};
        }
      }
    }
  }

  // Strength reduction

  std::unordered_set<OperandID> ivappeared_set;

  std::vector<BasicBlockID> ordered_bb_id_list;
  auto curr_bb = header_bb;
  while (true) {
    ordered_bb_id_list.push_back(curr_bb->id);

    bool is_last = true;

    for (auto succ_id : curr_bb->succ_list) {
      if (std::find(ordered_bb_id_list.begin(), ordered_bb_id_list.end(),
                    succ_id) != ordered_bb_id_list.end()) {
        continue;
      }

      if (loop_info.body_id_set.count(succ_id)) {
        curr_bb = builder.context.get_basic_block(succ_id);
        is_last = false;
        break;
      }
    }

    if (is_last) {
      break;
    }
  }

  for (auto bb_id : ordered_bb_id_list) {
    auto bb = builder.context.get_basic_block(bb_id);
    for (auto instr = bb->head_instruction->next; instr != bb->tail_instruction;
         instr = instr->next) {
      auto maybe_def_id = instr->maybe_def_id;
      if (maybe_def_id.has_value()) {
        auto def_id = maybe_def_id.value();
        auto maybe_iv = ivset.find_set(def_id);
        if (maybe_iv.has_value() && !instr->is_phi()) {
          // Skip indvar def instructions
          ivappeared_set.insert(maybe_iv.value());
          continue;
        }
      }

      bool has_indvar = false;
      bool can_reduce = true;

      // Check if all the uses satisfy the condition
      // 1. all the uses are either indvar or defined outside the loop (or
      // constant)

      for (auto use_id : instr->use_id_list) {
        auto use = builder.context.get_operand(use_id);
        if (ivset.find_set(use->id).has_value()) {
          has_indvar = true;
        } else if (use->maybe_def_id.has_value()) {
          auto def_instr =
            builder.context.get_instruction(use->maybe_def_id.value());
          if (loop_info.body_id_set.count(def_instr->parent_block_id)) {
            can_reduce = false;
            break;
          }
        }
      }

      if (!has_indvar || !can_reduce) {
        continue;
      }

      auto maybe_gep = instr->as<GetElementPtr>();

      if (maybe_gep.has_value()) {
        // Check condition
        // %t = gep [c * i32], [c * i32]* P, i32 0, i32 indvar
        // load %v, %t
        // ->
        // phi %t, P
        // ...
        // load %v, %t
        // %t = gep i32, ptr, i32 indvar

        auto gep = maybe_gep.value();
        auto dst = builder.context.get_operand(gep.dst_id);
        auto basis_type = gep.basis_type;

        // All the use of dst should be inside the current block
        bool can_reduce = true;
        for (auto use_id : dst->use_id_list) {
          auto use_instr = builder.context.get_instruction(use_id);
          if (use_instr->parent_block_id != bb_id) {
            can_reduce = false;
            break;
          }
        }

        if (!can_reduce) {
          continue;
        }

        if (basis_type->as<type::Array>().has_value()) {
          auto element_type =
            basis_type->as<type::Array>().value().element_type;
          if (element_type->as<type::Integer>().has_value()) {
            auto integer_type = element_type->as<type::Integer>().value();

            bool is_first_zero =
              builder.context.get_operand(gep.index_id_list[0])->is_zero();

            auto indvar = builder.context.get_operand(gep.index_id_list[1]);

            if (is_first_zero && 
                integer_type.size == 32 && 
                ivset.find_set(indvar->id).has_value()) {
              bool ivappeared =
                ivappeared_set.count(ivset.find_set(indvar->id).value());

              auto ivrecord = ivrecord_map[ivset.find_set(indvar->id).value()];

              auto ivstart = builder.context.get_operand(ivrecord.start_id);

              if (!ivstart->is_zero()) {
                continue;
              }

              auto old_ptr = builder.context.get_operand(gep.ptr_id);
              auto new_ptr_id = builder.fetch_arbitrary_operand(
                builder.fetch_pointer_type(builder.fetch_i32_type())
              );
              auto phi_ptr_id = builder.fetch_arbitrary_operand(
                builder.fetch_pointer_type(builder.fetch_i32_type())
              );

              builder.set_curr_basic_block(bb);

              auto step_id =
                ivrecord_map[ivset.find_set(indvar->id).value()].step_id;

              auto new_gep_instr = builder.fetch_getelementptr_instruction(
                new_ptr_id, builder.fetch_i32_type(), phi_ptr_id, {step_id}
              );

              if (ivappeared) {
                instr->insert_next(new_gep_instr);
              } else {
                bb->tail_instruction->prev.lock()->insert_prev(new_gep_instr);
              }

              auto use_id_list_copy = dst->use_id_list;

              for (auto use_id : use_id_list_copy) {
                auto use_instr = builder.context.get_instruction(use_id);
                if (!ivappeared) {
                  use_instr->replace_operand(
                    dst->id, phi_ptr_id, builder.context
                  );
                } else {
                  use_instr->replace_operand(
                    dst->id, new_ptr_id, builder.context
                  );
                }
              }

              auto next_instr = instr->next;
              instr->remove(builder.context);
              instr = next_instr;

              auto incoming_list =
                std::vector<std::tuple<OperandID, BasicBlockID>>();
              for (auto pred_id : header_bb->pred_list) {
                auto pred_bb = builder.context.get_basic_block(pred_id);
                if (loop_info.body_id_set.count(pred_id)) {
                  incoming_list.push_back(std::make_tuple(new_ptr_id, pred_id));
                  continue;
                }
                // Add a bitcast to i32* to the pred bb
                auto bitcast_ptr_id = builder.fetch_arbitrary_operand(
                  builder.fetch_pointer_type(builder.fetch_i32_type())
                );
                auto bitcast_instr = builder.fetch_cast_instruction(
                  CastOp::BitCast, bitcast_ptr_id, old_ptr->id
                );

                pred_bb->tail_instruction->prev.lock()->insert_prev(
                  bitcast_instr
                );

                incoming_list.push_back(std::make_tuple(bitcast_ptr_id, pred_id)
                );
              }
              builder.set_curr_basic_block(header_bb);
              auto phi_instr =
                builder.fetch_phi_instruction(phi_ptr_id, incoming_list);
              builder.prepend_instruction_to_curr_basic_block(phi_instr);
            }
          }
        }
      }
    }
  }
}

}  // namespace ir
}  // namespace syc