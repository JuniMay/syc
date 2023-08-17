#include "passes__ir__loop_unrolling.h"
#include "ir__basic_block.h"
#include "ir__builder.h"
#include "ir__function.h"
#include "ir__instruction.h"
#include "ir__operand.h"

namespace syc {
namespace ir {

void loop_unrolling(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    lcssa_transform(function, builder);
    // loop_unrolling_function(function, builder);
  }
}

void loop_unrolling_function(FunctionPtr function, Builder& builder) {
  builder.switch_function(function->name);

  bool unroll = true;

  while (unroll) {
    unroll = false;
    auto loop_opt_ctx = LoopOptContext();
    detect_natural_loop(function, builder, loop_opt_ctx);

    if (loop_opt_ctx.loop_info_map.size() > 50) {
      break;
    }

    // Sort loop_info_map by body size
    std::vector<LoopInfo> loop_info_list;
    for (auto [_, loop_info] : loop_opt_ctx.loop_info_map) {
      loop_info_list.push_back(loop_info);
    }

    std::sort(
      loop_info_list.begin(), loop_info_list.end(),
      [](const LoopInfo& lhs, const LoopInfo& rhs) {
        return lhs.body_id_set.size() < rhs.body_id_set.size();
      }
    );

    for (auto loop_info : loop_info_list) {
      unroll |= loop_unrolling_helper(loop_info, builder, loop_opt_ctx);
      if (unroll) {
        break;
      }
    }
  }
}

bool loop_unrolling_helper(
  LoopInfo& loop_info,
  Builder& builder,
  LoopOptContext& loop_opt_ctx
) {
  using namespace instruction;

  if (loop_info.exiting_id_set.size() > 1 || loop_info.exit_id_set.size() > 1) {
    return false;
  }

  // Constraint, header bb is the exiting bb.
  if (!loop_info.exiting_id_set.count(loop_info.header_id)) {
    return false;
  }

  if (loop_info.body_id_set.size() > 30) {
    return false;
  }

  auto header_bb = builder.context.get_basic_block(loop_info.header_id);

  OperandID loop_iv_id;
  OperandID loop_cond_id;

  int iv_st;
  int iv_ed;
  int iv_stride;

  bool do_loop_unrolling = false;

  for (auto instr = header_bb->head_instruction->next;
       instr != header_bb->tail_instruction && instr->is_phi();
       instr = instr->next) {
    std::optional<OperandID> maybe_loop_iv_id = std::nullopt;
    std::optional<OperandID> maybe_loop_cond_id = std::nullopt;
    std::optional<int> maybe_iv_st = std::nullopt;
    std::optional<int> maybe_iv_ed = std::nullopt;

    auto phi = instr->as<Phi>().value();

    if (phi.incoming_list.size() != 2) {
      continue;
    }

    std::optional<OperandID> maybe_alternate_id;
    std::optional<OperandID> maybe_start_id;

    for (auto [operand_id, block_id] : phi.incoming_list) {
      if (loop_info.body_id_set.count(block_id)) {
        maybe_alternate_id = operand_id;
      } else if (builder.context.get_operand(operand_id)->is_constant() && 
                 builder.context.get_operand(operand_id)->is_int()) {
        maybe_start_id = operand_id;
      }
    }

    if (!maybe_alternate_id || !maybe_start_id) {
      continue;
    }

    auto alternative_id = maybe_alternate_id.value();
    auto start_id = maybe_start_id.value();

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
      // TODO: support more indvars
      if (binary.op == BinaryOp::Add) {
        auto dst_id = binary.dst_id;
        auto lhs_id = binary.lhs_id;
        auto rhs_id = binary.rhs_id;

        auto rhs = builder.context.get_operand(rhs_id);

        if (lhs_id == phi.dst_id && dst_id == alternative_id && rhs->is_constant()) {
          auto constant = std::get<operand::ConstantPtr>(
            builder.context.get_operand(rhs_id)->kind
          );
          iv_stride = std::get<int>(constant->kind);
          maybe_loop_iv_id = phi.dst_id;
          auto start_constant = std::get<operand::ConstantPtr>(
            builder.context.get_operand(start_id)->kind
          );
          maybe_iv_st = std::get<int>(start_constant->kind);
        }
      } else {
        continue;
      }
    }

    // check icmp instruction
    auto phi_dst_operand = builder.context.get_operand(phi.dst_id);
    for (auto use_instr_id : phi_dst_operand->use_id_list) {
      auto use_instr = builder.context.get_instruction(use_instr_id);
      if (use_instr->parent_block_id != loop_info.header_id) {
        continue;
      }
      if (use_instr->is_icmp()) {
        auto icmp = use_instr->as<ICmp>().value();
        // TODO: add more icmp cases
        if (icmp.cond == ICmpCond::Slt) {
          auto lhs_id = icmp.lhs_id;
          auto rhs_id = icmp.rhs_id;
          auto rhs = builder.context.get_operand(rhs_id);
          if (rhs->is_constant()) {
            auto constant = std::get<operand::ConstantPtr>(rhs->kind);
            auto iv_ed = std::get<int>(constant->kind);
            maybe_iv_ed = iv_ed - 1;
            maybe_loop_cond_id = icmp.dst_id;
          }
        } else if (icmp.cond == ICmpCond::Sle) {
          auto lhs_id = icmp.lhs_id;
          auto rhs_id = icmp.rhs_id;
          auto rhs = builder.context.get_operand(rhs_id);
          if (rhs->is_constant()) {
            auto constant = std::get<operand::ConstantPtr>(rhs->kind);
            auto iv_ed = std::get<int>(constant->kind);
            maybe_iv_ed = iv_ed;
            maybe_loop_cond_id = icmp.dst_id;
          }
        } else {
          continue;
        }
      }
    }

    // if all three conditions are satisfied, then we can do loop unrolling
    if (maybe_loop_iv_id.has_value() && maybe_iv_st.has_value() && maybe_iv_ed.has_value()) {
      loop_iv_id = maybe_loop_iv_id.value();
      loop_cond_id = maybe_loop_cond_id.value();
      iv_st = maybe_iv_st.value();
      iv_ed = maybe_iv_ed.value();
      do_loop_unrolling = true;
      break;
    }
  }

  if ((iv_ed - iv_st) / iv_stride > 300 || iv_ed - iv_st <= 0) {
    return false;
  }

  if (do_loop_unrolling) {
    // remove phi, icmp, br instructions
    auto loop_iv_operand = builder.context.get_operand(loop_iv_id);
    auto loop_iv_def_instr =
      builder.context.get_instruction(loop_iv_operand->maybe_def_id.value());
    auto loop_cond_operand = builder.context.get_operand(loop_cond_id);
    auto loop_cond_def_instr =
      builder.context.get_instruction(loop_cond_operand->maybe_def_id.value());

    builder.set_curr_basic_block(header_bb);

    auto br_instr = header_bb->tail_instruction->prev.lock();
    auto then_bb_id = br_instr->as<CondBr>()->then_block_id;
    auto else_bb_id = br_instr->as<CondBr>()->else_block_id;

    auto then_bb = builder.context.get_basic_block(then_bb_id);
    auto else_bb = builder.context.get_basic_block(else_bb_id);

    std::vector<BasicBlockPtr> new_bb_list;
    auto curr_loop_unroll_ctx = LoopUnrollingContext();
    auto prev_loop_unroll_ctx = LoopUnrollingContext();

    curr_loop_unroll_ctx.loop_info = loop_info;
    prev_loop_unroll_ctx.loop_info = loop_info;

    // Map header id
    auto new_header_bb = builder.fetch_basic_block();
    curr_loop_unroll_ctx.basic_block_id_map[header_bb->id] = new_header_bb->id;
    new_bb_list.push_back(new_header_bb);

    BasicBlockPtr insert_pos_bb = nullptr;

    // Get initial mapping
    for (auto bb_id : loop_info.body_id_set) {
      auto bb = builder.context.get_basic_block(bb_id);
      for (auto instr = bb->head_instruction->next;
           instr != bb->tail_instruction; instr = instr->next) {
        if (instr->maybe_def_id.has_value()) {
          prev_loop_unroll_ctx.operand_id_map[instr->maybe_def_id.value()] =
            instr->maybe_def_id.value();
        }
      }
    }

    for (int loop_index = iv_st + iv_stride; loop_index <= iv_ed;
         loop_index += iv_stride) {
      // Map basic blocks
      for (auto bb_id : loop_info.body_id_set) {
        if (bb_id == header_bb->id) {
          continue;
        }
        auto new_bb = builder.fetch_basic_block();
        curr_loop_unroll_ctx.basic_block_id_map[bb_id] = new_bb->id;
        new_bb_list.push_back(new_bb);
      }

      BasicBlockID old_header_bb_id;

      for (auto bb_id : loop_info.body_id_set) {
        auto bb = builder.context.get_basic_block(bb_id);
        auto unroll_bb = builder.context.get_basic_block(
          curr_loop_unroll_ctx.basic_block_id_map.at(bb_id)
        );

        builder.set_curr_basic_block(unroll_bb);

        if (bb->id == header_bb->id) {
          // Skip phi instruction and re-map
          for (auto instr = bb->head_instruction->next;
               instr != bb->tail_instruction; instr = instr->next) {
            if (instr->is_phi()) {
              auto phi = instr->as<Phi>().value();
              for (auto [operand_id, block_id] : phi.incoming_list) {
                auto operand = builder.context.get_operand(operand_id);
                if (loop_info.body_id_set.count(block_id)) {
                  if (operand->maybe_def_id.has_value()) {
                    auto def_instr = builder.context.get_instruction(
                      operand->maybe_def_id.value()
                    );
                    if (!loop_info.body_id_set.count(def_instr->parent_block_id
                        )) {
                      curr_loop_unroll_ctx.operand_id_map[phi.dst_id] =
                        operand_id;
                    } else {
                      curr_loop_unroll_ctx.operand_id_map[phi.dst_id] =
                        prev_loop_unroll_ctx.operand_id_map.at(operand_id);
                    }
                  } else {
                    curr_loop_unroll_ctx.operand_id_map[phi.dst_id] =
                      operand_id;
                  }
                }
              }
            } else {
              auto new_instr =
                clone_instruction(instr, builder, curr_loop_unroll_ctx);
              builder.append_instruction(new_instr);
            }
          }
          // Actually record the exiting bb
          old_header_bb_id = unroll_bb->id;
          // Map header bb, the mapped header is for next iteration
          auto new_header_bb = builder.fetch_basic_block();
          curr_loop_unroll_ctx.basic_block_id_map[header_bb->id] =
            new_header_bb->id;
          new_bb_list.push_back(new_header_bb);
        } else {
          // Directly map
          for (auto instr = bb->head_instruction->next;
               instr != bb->tail_instruction; instr = instr->next) {
            auto new_instr =
              clone_instruction(instr, builder, curr_loop_unroll_ctx);
            builder.append_instruction(new_instr);
          }
        }
      }

      // Patch exit bb
      auto exit_bb =
        builder.context.get_basic_block(*loop_info.exit_id_set.begin());

      for (auto instr = exit_bb->head_instruction->next;
           instr != exit_bb->tail_instruction && instr->is_phi();
           instr = instr->next) {
        auto phi = instr->as<Phi>().value();
        auto incoming_list_copy = phi.incoming_list;
        for (auto [operand_id, block_id] : incoming_list_copy) {
          if (loop_info.body_id_set.count(block_id)) {
            instr->add_phi_operand(
              curr_loop_unroll_ctx.operand_id_map.at(operand_id),
              old_header_bb_id, builder.context
            );
          }
        }
      }

      prev_loop_unroll_ctx.operand_id_map = curr_loop_unroll_ctx.operand_id_map;
      prev_loop_unroll_ctx.basic_block_id_map =
        curr_loop_unroll_ctx.basic_block_id_map;

      curr_loop_unroll_ctx.operand_id_map.clear();
      curr_loop_unroll_ctx.basic_block_id_map.clear();

      curr_loop_unroll_ctx.basic_block_id_map[header_bb->id] =
        prev_loop_unroll_ctx.basic_block_id_map.at(header_bb->id);
    }

    // Patch back the initial condbr
    for (auto bb_id : loop_info.body_id_set) {
      auto bb = builder.context.get_basic_block(bb_id);
      for (auto instr = bb->head_instruction->next;
           instr != bb->tail_instruction; instr = instr->next) {
        if (instr->is_br()) {
          auto& br = instr->as_ref<Br>().value().get();
          if (br.block_id == header_bb->id) {
            br.block_id = new_header_bb->id;
            new_header_bb->add_use(instr->id);
            header_bb->remove_use(instr->id);
            insert_pos_bb = bb;
            bb->remove_succ(header_bb->id);
            bb->add_succ(new_header_bb->id);
            new_header_bb->add_pred(bb->id);
            header_bb->remove_pred(bb->id);
          }
        } else if (instr->as<CondBr>().has_value()) {
          auto& cond_br = instr->as_ref<CondBr>().value().get();
          if (cond_br.then_block_id == header_bb->id) {
            cond_br.then_block_id = new_header_bb->id;
            new_header_bb->add_use(instr->id);
            header_bb->remove_use(instr->id);
            insert_pos_bb = bb;
            bb->remove_succ(header_bb->id);
            bb->add_succ(new_header_bb->id);
            new_header_bb->add_pred(bb->id);
            header_bb->remove_pred(bb->id);
          } else if (cond_br.else_block_id == header_bb->id) {
            cond_br.else_block_id = new_header_bb->id;
            new_header_bb->add_use(instr->id);
            header_bb->remove_use(instr->id);
            insert_pos_bb = bb;
            bb->remove_succ(header_bb->id);
            bb->add_succ(new_header_bb->id);
            new_header_bb->add_pred(bb->id);
            header_bb->remove_pred(bb->id);
          }
        }
      }
    }

    // One last header
    auto new_header_bb_id =
      prev_loop_unroll_ctx.basic_block_id_map.at(header_bb->id);
    new_header_bb = builder.context.get_basic_block(new_header_bb_id);
    builder.set_curr_basic_block(new_header_bb);

    for (auto instr = header_bb->head_instruction->next;
         instr != header_bb->tail_instruction && instr->is_phi();
         instr = instr->next) {
      auto phi = instr->as<Phi>().value();

      for (auto [operand_id, block_id] : phi.incoming_list) {
        auto operand = builder.context.get_operand(operand_id);
        if (loop_info.body_id_set.count(block_id)) {
          if (operand->maybe_def_id.has_value()) {
            auto def_instr =
              builder.context.get_instruction(operand->maybe_def_id.value());
            if (!loop_info.body_id_set.count(def_instr->parent_block_id)) {
              prev_loop_unroll_ctx.operand_id_map[phi.dst_id] = operand_id;
            } else {
              prev_loop_unroll_ctx.operand_id_map[phi.dst_id] =
                prev_loop_unroll_ctx.operand_id_map.at(operand_id);
            }
          } else {
            prev_loop_unroll_ctx.operand_id_map[phi.dst_id] = operand_id;
          }
        }
      }

      for (auto [operand_id, block_id] : phi.incoming_list) {
        if (!loop_info.body_id_set.count(block_id)) {
          auto phi_dst = builder.context.get_operand(phi.dst_id);
          auto use_id_list_copy = phi_dst->use_id_list;
          for (auto use_id : use_id_list_copy) {
            auto use_instr = builder.context.get_instruction(use_id);
            if (loop_info.body_id_set.count(use_instr->parent_block_id)) {
              use_instr->replace_operand(
                phi.dst_id, operand_id, builder.context
              );
            }
          }
        }
      }
    }

    // clean up header_bb's phi
    for (auto instr = header_bb->head_instruction->next;
         instr != header_bb->tail_instruction && instr->is_phi();
         instr = instr->next) {
      auto phi = instr->as<Phi>().value();
      auto incoming_list_copy = phi.incoming_list;
      for (auto [_, block_id] : incoming_list_copy) {
        if (std::find(header_bb->pred_list.begin(),
                      header_bb->pred_list.end(),
                      block_id) == header_bb->pred_list.end()) {
          instr->remove_phi_operand(block_id, builder.context);
        }
      }
    }

    // Patch the last jump (from exiting block, i.e. header bb) to the exit bb
    auto exit_bb =
      builder.context.get_basic_block(*loop_info.exit_id_set.begin());

    for (auto instr = exit_bb->head_instruction->next;
         instr != exit_bb->tail_instruction && instr->is_phi();
         instr = instr->next) {
      auto phi = instr->as<Phi>().value();
      auto incoming_list_copy = phi.incoming_list;
      for (auto [operand_id, block_id] : incoming_list_copy) {
        if (loop_info.body_id_set.count(block_id)) {
          instr->add_phi_operand(
            prev_loop_unroll_ctx.operand_id_map.at(operand_id),
            new_header_bb_id, builder.context
          );
        }
      }
    }

    auto new_br_instr = builder.fetch_br_instruction(else_bb_id);
    builder.append_instruction(new_br_instr);

    // Modify code, append blocks
    if (insert_pos_bb != nullptr) {
      for (auto bb : new_bb_list) {
        insert_pos_bb->insert_next(bb);
        insert_pos_bb = bb;
      }
    }

    return true;
  }
  return false;
}

OperandID clone_operand(
  OperandID operand_id,
  Builder& builder,
  LoopUnrollingContext& context
) {
  if (context.operand_id_map.count(operand_id)) {
    return context.operand_id_map[operand_id];
  }
  auto operand = builder.context.get_operand(operand_id);
  if (operand->maybe_def_id.has_value()) {
    auto def_instr_id = operand->maybe_def_id.value();
    auto def_instr = builder.context.get_instruction(def_instr_id);
    auto def_parent_bb_id = def_instr->parent_block_id;
    if (!context.loop_info.body_id_set.count(def_parent_bb_id)) {
      // if the operand is not defined in the loop body, don't clone it
      return operand_id;
    }
  }
  if (operand->is_arbitrary()) {
    auto new_operand_id = builder.fetch_operand(operand->type, operand->kind);
    context.operand_id_map[operand_id] = new_operand_id;
    return new_operand_id;
  } else {
    return operand_id;
  }
}

InstructionPtr clone_instruction(
  InstructionPtr instruction,
  Builder& builder,
  LoopUnrollingContext& context
) {
  using namespace instruction;
  return std::visit(
    overloaded{
      [&](const Binary& binary) -> InstructionPtr {
        auto dst_id = clone_operand(binary.dst_id, builder, context);
        auto lhs_id = clone_operand(binary.lhs_id, builder, context);
        auto rhs_id = clone_operand(binary.rhs_id, builder, context);

        auto new_instruction =
          builder.fetch_binary_instruction(binary.op, dst_id, lhs_id, rhs_id);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const ICmp& icmp) -> InstructionPtr {
        auto dst_id = clone_operand(icmp.dst_id, builder, context);
        auto lhs_id = clone_operand(icmp.lhs_id, builder, context);
        auto rhs_id = clone_operand(icmp.rhs_id, builder, context);

        auto new_instruction =
          builder.fetch_icmp_instruction(icmp.cond, dst_id, lhs_id, rhs_id);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const FCmp& fcmp) -> InstructionPtr {
        auto dst_id = clone_operand(fcmp.dst_id, builder, context);
        auto lhs_id = clone_operand(fcmp.lhs_id, builder, context);
        auto rhs_id = clone_operand(fcmp.rhs_id, builder, context);

        auto new_instruction =
          builder.fetch_fcmp_instruction(fcmp.cond, dst_id, lhs_id, rhs_id);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Cast& cast) -> InstructionPtr {
        auto dst_id = clone_operand(cast.dst_id, builder, context);
        auto src_id = clone_operand(cast.src_id, builder, context);

        auto new_instruction =
          builder.fetch_cast_instruction(cast.op, dst_id, src_id);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const CondBr& condbr) -> InstructionPtr {
        auto cond_id = clone_operand(condbr.cond_id, builder, context);

        auto then_block_id = condbr.then_block_id;
        auto else_block_id = condbr.else_block_id;

        if (context.basic_block_id_map.count(then_block_id)) {
          then_block_id = context.basic_block_id_map.at(then_block_id);
        }
        if (context.basic_block_id_map.count(else_block_id)) {
          else_block_id = context.basic_block_id_map.at(else_block_id);
        }

        auto new_instruction = builder.fetch_condbr_instruction(
          cond_id, then_block_id, else_block_id
        );

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Br& br) -> InstructionPtr {
        auto block_id = br.block_id;

        if (context.basic_block_id_map.count(block_id)) {
          block_id = context.basic_block_id_map.at(block_id);
        }

        auto new_instruction = builder.fetch_br_instruction(block_id);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Phi& phi) -> InstructionPtr {
        auto dst_id = clone_operand(phi.dst_id, builder, context);

        std::vector<std::tuple<OperandID, BasicBlockID>> incoming_list;
        for (auto [incoming_operand_id, incoming_block_id] :
             phi.incoming_list) {
          auto new_incoming_block_id = incoming_block_id;

          if (context.basic_block_id_map.count(incoming_block_id)) {
            new_incoming_block_id =
              context.basic_block_id_map.at(incoming_block_id);
          }

          incoming_list.push_back(std::make_tuple(
            clone_operand(incoming_operand_id, builder, context),
            new_incoming_block_id
          ));
        }

        auto new_instruction =
          builder.fetch_phi_instruction(dst_id, incoming_list);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Call& call) -> InstructionPtr {
        std::optional<OperandID> maybe_dst_id = std::nullopt;
        if (call.maybe_dst_id.has_value()) {
          maybe_dst_id =
            clone_operand(call.maybe_dst_id.value(), builder, context);
        }

        std::vector<OperandID> arg_id_list;
        for (auto arg_id : call.arg_id_list) {
          arg_id_list.push_back(clone_operand(arg_id, builder, context));
        }

        auto new_instruction = builder.fetch_call_instruction(
          maybe_dst_id, call.function_name, arg_id_list
        );

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Alloca& alloca) -> InstructionPtr {
        auto dst_id = clone_operand(alloca.dst_id, builder, context);
        std::optional<OperandID> maybe_size_id = std::nullopt;
        std::optional<OperandID> maybe_align_id = std::nullopt;
        std::optional<OperandID> maybe_addrspace_id = std::nullopt;
        if (alloca.maybe_size_id.has_value()) {
          maybe_size_id =
            clone_operand(alloca.maybe_size_id.value(), builder, context);
        }
        if (alloca.maybe_align_id.has_value()) {
          maybe_align_id =
            clone_operand(alloca.maybe_align_id.value(), builder, context);
        }
        if (alloca.maybe_addrspace_id.has_value()) {
          maybe_addrspace_id =
            clone_operand(alloca.maybe_addrspace_id.value(), builder, context);
        }

        auto new_instruction = builder.fetch_alloca_instruction(
          dst_id, alloca.allocated_type, maybe_size_id, maybe_align_id,
          maybe_addrspace_id, false
        );

        // This should be updated after append and prepend.
        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Load& load) -> InstructionPtr {
        auto dst_id = clone_operand(load.dst_id, builder, context);
        auto ptr_id = clone_operand(load.ptr_id, builder, context);

        std::optional<OperandID> maybe_align_id = std::nullopt;
        if (load.maybe_align_id.has_value()) {
          maybe_align_id =
            clone_operand(load.maybe_align_id.value(), builder, context);
        }

        auto new_instruction =
          builder.fetch_load_instruction(dst_id, ptr_id, maybe_align_id);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Store& store) -> InstructionPtr {
        auto value_id = clone_operand(store.value_id, builder, context);
        auto ptr_id = clone_operand(store.ptr_id, builder, context);

        std::optional<OperandID> maybe_align_id = std::nullopt;
        if (store.maybe_align_id.has_value()) {
          maybe_align_id =
            clone_operand(store.maybe_align_id.value(), builder, context);
        }

        auto new_instruction =
          builder.fetch_store_instruction(value_id, ptr_id, maybe_align_id);

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const GetElementPtr& gep) -> InstructionPtr {
        auto dst_id = clone_operand(gep.dst_id, builder, context);
        auto ptr_id = clone_operand(gep.ptr_id, builder, context);

        std::vector<OperandID> index_id_list;
        for (auto index_id : gep.index_id_list) {
          index_id_list.push_back(clone_operand(index_id, builder, context));
        }

        auto new_instruction = builder.fetch_getelementptr_instruction(
          dst_id, gep.basis_type, ptr_id, index_id_list
        );

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const auto& kind) -> InstructionPtr { return nullptr; },
    },
    instruction->kind
  );
}

}  // namespace ir
}  // namespace syc