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
    lcssa_transform(function, builder);
    loop_indvar_simplify_function(function, builder);
  }
}

void loop_indvar_simplify_function(FunctionPtr function, Builder& builder) {
  auto loop_opt_ctx = LoopOptContext();
  detect_natural_loop(function, builder, loop_opt_ctx);

  std::set<BasicBlockID> to_be_removed_bb_id_list = {};

  for (auto [_, loop_info] : loop_opt_ctx.loop_info_map) {
    loop_indvar_simplify_helper(loop_info, builder, to_be_removed_bb_id_list);
  }

  for (auto bb_id : to_be_removed_bb_id_list) {
    auto bb = builder.context.get_basic_block(bb_id);
    bb->remove(builder.context);
  }
}

void loop_indvar_simplify_helper(
  LoopInfo& loop_info,
  Builder& builder,
  std::set<BasicBlockID>& to_be_removed_bb_id_list
) {
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
  std::unordered_map<OperandID, OperandID> iv_phi_dst_map;
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
          ivrecord_map[representative] = IvRecord{
            representative, start_id,     binary.op,
            rhs_id,         std::nullopt, {def_instr->id, instr->id},
          };
          iv_before_map[representative] = phi.dst_id;
          iv_phi_dst_map[representative] = phi.dst_id;
        }
      } else if (binary.op == BinaryOp::SRem) {
        auto srem_dst = builder.context.get_operand(binary.dst_id);
        auto srem_lhs = builder.context.get_operand(binary.lhs_id);
        auto srem_rhs = builder.context.get_operand(binary.rhs_id);

        if (srem_rhs->is_constant() && srem_lhs->maybe_def_id.has_value()) {
          auto srem_lhs_def =
            builder.context.get_instruction(srem_lhs->maybe_def_id.value());
          if (srem_lhs_def->as<Binary>().has_value()) {
            auto srem_lhs_binary = srem_lhs_def->as<Binary>().value();
            auto srem_lhs_binary_op = srem_lhs_binary.op;
            auto srem_lhs_lhs =
              builder.context.get_operand(srem_lhs_binary.lhs_id);
            auto srem_lhs_rhs =
              builder.context.get_operand(srem_lhs_binary.rhs_id);

            if (srem_lhs_binary_op == BinaryOp::Add &&
              srem_lhs_lhs->id == phi.dst_id && srem_lhs_rhs->is_constant()) {
              ivset.make_set(phi.dst_id);
              ivset.make_set(alternative_id);
              ivset.union_set(phi.dst_id, alternative_id);

              auto representative = ivset.find_set(phi.dst_id).value();
              ivrecord_map[representative] = IvRecord{
                representative,
                start_id,
                srem_lhs_binary_op,
                srem_lhs_rhs->id,
                srem_rhs->id,
                {def_instr->id, instr->id, srem_lhs_def->id},
              };
              iv_before_map[representative] = phi.dst_id;
              iv_phi_dst_map[representative] = phi.dst_id;
            }
          }
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

  // Strength reduction
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

  // Simplify
  std::optional<IvRecord> maybe_base_ivrecord = std::nullopt;

  for (auto [indvar_id, ivrecord] : ivrecord_map) {
    auto iv_stride = builder.context.get_operand(ivrecord.step_id);
    if (iv_stride->is_constant() && !ivrecord.maybe_modulo.has_value()) {
      auto stride = std::get<operand::ConstantPtr>(iv_stride->kind);
      auto stride_val = std::get<int>(stride->kind);
      if (stride_val == 1) {
        maybe_base_ivrecord = ivrecord;
        break;
      }
    }
  }

  if (!maybe_base_ivrecord.has_value()) {
    return;
  }

  auto base_ivrecord = maybe_base_ivrecord.value();

  // DEBUG
  std::cout << "base_ivrecord: " << base_ivrecord.indvar_id << std::endl;
  std::cout << "  start_id: "
            << builder.context.get_operand(base_ivrecord.start_id)->to_string()
            << std::endl;
  std::cout << "  step_id: "
            << builder.context.get_operand(base_ivrecord.step_id)->to_string()
            << std::endl;

  for (auto [indvar_id, ivrecord] : ivrecord_map) {
    auto iv_st_operand = builder.context.get_operand(ivrecord.start_id);
    auto iv_stride_operand = builder.context.get_operand(ivrecord.step_id);
    auto induction_op = ivrecord.op;

    auto maybe_modulo = ivrecord.maybe_modulo;

    // DEBUG
    std::cout << "indvar_id: " << indvar_id << std::endl;
    std::cout << "  start_id: " << iv_st_operand->to_string() << std::endl;
    std::cout << "  step_id: " << iv_stride_operand->to_string() << std::endl;
    if (maybe_modulo.has_value()) {
      std::cout
        << "  modulo: "
        << builder.context.get_operand(maybe_modulo.value())->to_string()
        << std::endl;
    }

    if (indvar_id == base_ivrecord.indvar_id) {
      continue;
    }

    // Assume not effective
    if (loop_info.exit_id_set.size() != 1) {
      continue;
    }

    // DEBUG
    std::cout << "exit_id: " << *loop_info.exit_id_set.begin() << std::endl;

    auto exit_bb =
      builder.context.get_basic_block(*loop_info.exit_id_set.begin());
    if (exit_bb->pred_list.size() != 1) {
      continue;
    }

    if (iv_stride_operand->is_constant()) {
      // Find the use of the indvar in exit block
      std::optional<OperandID> lcssa_phi_dst_id = std::nullopt;
      for (auto instr = exit_bb->head_instruction->next;
           instr != exit_bb->tail_instruction && instr->is_phi();
           instr = instr->next) {
        auto [operand_id, bb_id] = instr->as<Phi>().value().incoming_list[0];
        auto maybe_indvar_id = ivset.find_set(operand_id);
        if (!maybe_indvar_id.has_value()) {
          continue;
        }
        // Found the operand
        if (maybe_indvar_id.value() == indvar_id) {
          lcssa_phi_dst_id = instr->maybe_def_id.value();
          break;
        }
      }
      if (lcssa_phi_dst_id.has_value()) {
        auto lcssa_phi_dst =
          builder.context.get_operand(lcssa_phi_dst_id.value());
        auto insert_pos = exit_bb->head_instruction->next;
        while (insert_pos->is_phi()) {
          insert_pos = insert_pos->next;
        }

        insert_pos = insert_pos->prev.lock();

        builder.set_curr_basic_block(exit_bb);

        OperandID new_dst_id;

        auto mul_dst_id =
          builder.fetch_arbitrary_operand(builder.fetch_i32_type());
        auto mul_instr = builder.fetch_binary_instruction(
          BinaryOp::Mul, mul_dst_id, iv_phi_dst_map.at(base_ivrecord.indvar_id),
          ivrecord.step_id, ivrecord.maybe_modulo.has_value()
        );

        insert_pos->insert_next(mul_instr);
        insert_pos = mul_instr;
        new_dst_id = mul_dst_id;

        if (ivrecord.maybe_modulo.has_value()) {
          auto modulo_id = ivrecord.maybe_modulo.value();
          auto srem_dst_id =
            builder.fetch_arbitrary_operand(builder.fetch_i32_type());

          auto srem_instr = builder.fetch_binary_instruction(
            BinaryOp::SRem, srem_dst_id, mul_dst_id, modulo_id, true
          );

          insert_pos->insert_next(srem_instr);
          new_dst_id = srem_dst_id;
        }

        auto use_id_list_copy = lcssa_phi_dst->use_id_list;
        for (auto use_id : use_id_list_copy) {
          auto use_instr = builder.context.get_instruction(use_id);
          use_instr->replace_operand(
            lcssa_phi_dst->id, new_dst_id, builder.context
          );
        }

        for (auto instr_id : ivrecord.iv_instr_id_list) {
          auto instr = builder.context.get_instruction(instr_id);
          instr->remove(builder.context);
        }
      }
    }
  }

  if (loop_info.exit_id_set.size() == 1) {
    auto exit_bb =
      builder.context.get_basic_block(*loop_info.exit_id_set.begin());

    for (auto instr_id : base_ivrecord.iv_instr_id_list) {
      auto instr = builder.context.get_instruction(instr_id);
      if (instr->is_phi()) {
        auto phi = instr->as<Phi>().value();
        auto dst = builder.context.get_operand(phi.dst_id);
        auto use_id_list_copy = dst->use_id_list;
        for (auto use_id : use_id_list_copy) {
          auto use_instr = builder.context.get_instruction(use_id);
          if (use_instr->is_icmp()) {
            auto icmp = use_instr->as<ICmp>().value();
            auto cond = icmp.cond;
            auto lhs = builder.context.get_operand(icmp.lhs_id);
            auto rhs = builder.context.get_operand(icmp.rhs_id);
            if (cond == ICmpCond::Slt) {
              if (lhs->id == dst->id && rhs->maybe_def_id.has_value()) {
                auto rhs_def_instr =
                  builder.context.get_instruction(rhs->maybe_def_id.value());
                if (!loop_info.body_id_set.count(rhs_def_instr->parent_block_id
                    )) {
                  auto use_id_list_copy = dst->use_id_list;
                  for (auto use_id : use_id_list_copy) {
                    auto use_instr = builder.context.get_instruction(use_id);
                    if (use_instr->parent_block_id == exit_bb->id) {
                      use_instr->replace_operand(
                        dst->id, rhs->id, builder.context
                      );
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    bool can_remove = true;

    for (auto instr_id : base_ivrecord.iv_instr_id_list) {
      auto instr = builder.context.get_instruction(instr_id);
      if (!instr->maybe_def_id.has_value()) {
        continue;
      }
      auto def = builder.context.get_operand(instr->maybe_def_id.value());
      for (auto use_id : def->use_id_list) {
        auto use_instr = builder.context.get_instruction(use_id);
        if (!loop_info.body_id_set.count(use_instr->parent_block_id)) {
          can_remove = false;
          break;
        }
      }
    }

    // If the loop is not used outside, and with only indvar-related
    // instructions remove the loop
    for (auto bb_id : loop_info.body_id_set) {
      auto bb = builder.context.get_basic_block(bb_id);
      for (auto instr = bb->head_instruction->next;
           instr != bb->tail_instruction; instr = instr->next) {
        if (instr->is_br() || instr->as<CondBr>().has_value() || instr->as<ICmp>().has_value()) {
          continue;
        }
        if (std::find(
          base_ivrecord.iv_instr_id_list.begin(),
          base_ivrecord.iv_instr_id_list.end(),
          instr->id
        ) != base_ivrecord.iv_instr_id_list.end()) {
          continue;
        }
        std::cout << "CANNOT REMOVE: " << instr->to_string(builder.context)
                  << std::endl;
        can_remove = false;
        break;
      }
    }

    if (can_remove) {
      std::cout << "remove!" << std::endl;
      if (maybe_preheader_id.has_value() && header_bb->pred_list.size() == 2) {
        auto preheader_bb =
          builder.context.get_basic_block(maybe_preheader_id.value());
        auto branch_instr = preheader_bb->tail_instruction->prev.lock();
        std::cout << branch_instr->to_string(builder.context) << std::endl;
        if (branch_instr->is_br()) {
          auto& br = branch_instr->as_ref<Br>().value().get();
          if (br.block_id == header_bb->id) {
            br.block_id = exit_bb->id;
          }
        } else if (branch_instr->as<CondBr>().has_value()) {
          auto& condbr = branch_instr->as_ref<CondBr>().value().get();
          if (condbr.then_block_id == header_bb->id) {
            condbr.then_block_id = exit_bb->id;
          } else if (condbr.else_block_id == header_bb->id) {
            condbr.else_block_id = exit_bb->id;
          }
        }

        preheader_bb->remove_succ(header_bb->id);
        preheader_bb->add_succ(exit_bb->id);
        header_bb->remove_pred(preheader_bb->id);
        exit_bb->remove_pred(header_bb->id);
        exit_bb->add_pred(preheader_bb->id);

        for (auto bb_id : loop_info.body_id_set) {
          to_be_removed_bb_id_list.insert(bb_id);
        }
      }
    }
  }
}

}  // namespace ir
}  // namespace syc
