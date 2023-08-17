#include "passes/ir/loop_indvar_simplify.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "passes/adt.h"

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
  // Record the operand id of iv before update (dst of phi)
  std::unordered_map<OperandID, OperandID> iv_before_map;

  // Only find simple indvars
  // 1. Appear in the phi instruction of the loop header, with one value coming
  // from inside the loop and the other outside.
  // 2. The value coming from inside the loop is defined by add instruction.

  // Find all phi instructions in the loop header
  auto header_bb = builder.context.get_basic_block(loop_info.header_id);
  std::optional<BasicBlockID> maybe_preheader_id;
  for (auto instr = header_bb->head_instruction->next;
       instr != header_bb->tail_instruction && instr->is_phi();
       instr = instr->next) {
    auto phi = instr->as<Phi>().value();

    // Only accept phi with two incoming values
    if (phi.incoming_list.size() != 2) {
      continue;
    }

    std::optional<OperandID> maybe_start_id;
    std::optional<OperandID> maybe_alternative_id;

    for (auto [operand_id, block_id] : phi.incoming_list) {
      if (loop_info.body_id_set.count(block_id)) {
        maybe_alternative_id = operand_id;
      } else {
        maybe_start_id = operand_id;
        maybe_preheader_id = block_id;
      }
    }

    if (!maybe_alternative_id || !maybe_start_id || !maybe_preheader_id) {
      continue;
    }

    auto alternative_id = maybe_alternative_id.value();
    auto start_id = maybe_start_id.value();

    auto alternative_operand = builder.context.get_operand(alternative_id);

    if (!alternative_operand->maybe_def_id.has_value()) {
      continue;
    }

    auto def_instr =
      builder.context.get_instruction(alternative_operand->maybe_def_id.value()
      );

    // Indvar be like:
    // %t0 = phi [%st, preheader], [%t1, body]
    // ...
    // %t1 = add %t0, 1

    auto maybe_binary = def_instr->as<Binary>();

    if (maybe_binary.has_value()) {
      auto binary = maybe_binary.value();
      // Only support add yet
      if (binary.op == BinaryOp::Add) {
        auto dst_id = binary.dst_id;
        auto lhs_id = binary.lhs_id;
        auto rhs_id = binary.rhs_id;

        auto rhs = builder.context.get_operand(rhs_id);

        if (lhs_id == phi.dst_id && dst_id == alternative_id && rhs->is_constant()) {
          ivset.make_set(phi.dst_id);
          ivset.make_set(alternative_id);
          ivset.union_set(phi.dst_id, alternative_id);

          auto representative = ivset.find_set(phi.dst_id).value();
          ivrecord_map[representative] =
            IvRecord{representative, start_id, binary.op, rhs_id};
          iv_before_map[representative] = phi.dst_id;
        }
      }
    }
  }

  // The sequence of basic blocks in the loop body
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
      auto maybe_gep = instr->as<GetElementPtr>();

      if (maybe_gep.has_value()) {
        // Check condition
        //   %t = gep [c * i32], ptr P, i32 0, i32 indvar
        //   load %v, %t
        // ->
        //   phi %t, P
        //   ...
        //   load %v, %t
        //   ...
        //   %t = gep i32, ptr, i32 step
        // Note that if the use instr appears after the update instr, then use
        // new %t, other wise use P

        auto gep = maybe_gep.value();
        auto dst = builder.context.get_operand(gep.dst_id);
        auto ptr = builder.context.get_operand(gep.ptr_id);
        auto basis_type = gep.basis_type;

        // Check ptr outside the loop
        if (ptr->maybe_def_id.has_value()) {
          auto ptr_def_instr =
            builder.context.get_instruction(ptr->maybe_def_id.value());
          if (loop_info.body_id_set.count(ptr_def_instr->parent_block_id)) {
            continue;
          }
        }

        // Check index list and types
        std::optional<OperandID> maybe_iv_id = std::nullopt;

        for (size_t i = 0; i < gep.index_id_list.size() - 1; i++) {
          if (basis_type->as<type::Array>().has_value()) {
            basis_type = basis_type->as<type::Array>().value().element_type;
          }
        }

        // Check index list: the non-last indexers are zero, the last one is iv
        bool all_zero = true;
        std::vector<OperandID> init_index_id_list = {};
        for (size_t i = 0; i < gep.index_id_list.size() - 1; i++) {
          auto indexer = builder.context.get_operand(gep.index_id_list[i]);
          init_index_id_list.push_back(indexer->id);
          all_zero &= indexer->is_zero();
        }

        maybe_iv_id = ivset.find_set(gep.index_id_list.back());

        if (!maybe_iv_id.has_value()) {
          continue;
        }

        auto ivrecord = ivrecord_map.at(maybe_iv_id.value());
        auto iv_before_id = iv_before_map.at(maybe_iv_id.value());

        bool using_updated_iv = iv_before_id != gep.index_id_list.back();

        auto ivstart = builder.context.get_operand(ivrecord.start_id);

        init_index_id_list.push_back(ivrecord.start_id);
        all_zero &= ivstart->is_zero();

        // If not all zero, a gep needs to be inserted into the preheader
        if (!all_zero) {
          auto preheader =
            builder.context.get_basic_block(maybe_preheader_id.value());
          builder.set_curr_basic_block(preheader);
          auto insert_pos = preheader->tail_instruction->prev.lock();
          while (insert_pos->is_terminator() &&
                 insert_pos != preheader->head_instruction) {
            insert_pos = insert_pos->prev.lock();
          }
          auto new_ptr_id =
            builder.fetch_arbitrary_operand(builder.fetch_pointer_type());
          auto new_gep_instr = builder.fetch_getelementptr_instruction(
            new_ptr_id, gep.basis_type, ptr->id, init_index_id_list
          );
          insert_pos->insert_next(new_gep_instr);
          ptr = builder.context.get_operand(new_ptr_id);
        }

        // Phi dst
        auto phi_dst_ptr_id =
          builder.fetch_arbitrary_operand(builder.fetch_pointer_type());
        // New gep dst
        auto new_gep_dst_id =
          builder.fetch_arbitrary_operand(builder.fetch_pointer_type());

        // Construct phi instruction
        auto incoming_list = std::vector<std::tuple<OperandID, BasicBlockID>>();
        for (auto pred_id : header_bb->pred_list) {
          if (loop_info.body_id_set.count(pred_id)) {
            incoming_list.push_back(std::make_tuple(new_gep_dst_id, pred_id));
          } else {
            incoming_list.push_back(std::make_tuple(ptr->id, pred_id));
          }
        }
        builder.set_curr_basic_block(header_bb);
        auto phi_instr =
          builder.fetch_phi_instruction(phi_dst_ptr_id, incoming_list);
        builder.prepend_instruction_to_curr_basic_block(phi_instr);

        // Construct new gep instruction
        builder.set_curr_basic_block(bb);
        auto new_gep_instr = builder.fetch_getelementptr_instruction(
          new_gep_dst_id, basis_type, phi_dst_ptr_id, {ivrecord.step_id}
        );
        instr->insert_next(new_gep_instr);

        // Replace use of dst
        auto use_id_list_copy = dst->use_id_list;
        for (auto use_id : use_id_list_copy) {
          auto use_instr = builder.context.get_instruction(use_id);
          if (using_updated_iv) {
            use_instr->replace_operand(
              dst->id, new_gep_dst_id, builder.context
            );
          } else {
            use_instr->replace_operand(
              dst->id, phi_dst_ptr_id, builder.context
            );
          }
        }

        instr->remove(builder.context);
        instr = new_gep_instr;
      }
    }
  }
}

}  // namespace ir
}  // namespace syc