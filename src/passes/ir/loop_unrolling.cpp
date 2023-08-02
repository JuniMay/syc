#include "passes/ir/loop_unrolling.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void loop_unrolling(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    loop_unrolling_function(function, builder);
  }
}

void loop_unrolling_function(FunctionPtr function, Builder& builder) {
  auto loop_opt_ctx = LoopOptContext();
  detect_natural_loop(function, builder, loop_opt_ctx);

  for (auto [_, loop_info] : loop_opt_ctx.loop_info_map) {
    // std::cout << "header: " << loop_info.header_id << std::endl;
    // std::cout << "body: ";
    // for (auto body_id : loop_info.body_id_set) {
    //   std::cout << body_id << " ";
    // }
    // std::cout << std::endl;
    // std::cout << "exiting: ";
    // for (auto exiting_id : loop_info.exiting_id_set) {
    //   std::cout << exiting_id << " ";
    // }
    // std::cout << std::endl;
    loop_unrolling_helper(loop_info, builder);
  }
}

void loop_unrolling_helper(LoopInfo& loop_info, Builder& builder) {
  using namespace instruction;

  // // Check if there are no branches in the loop
  // for (auto body_id : loop_info.body_id_set) {
  //   auto bb = builder.context.get_basic_block(body_id);
  //   if (bb->succ_list.size() > 1) {
  //     // If more than one succ point to the bb inside the loop, then there is
  //     // a branch inside the loop
  //     // Check how many succs are inside the loop body.
  //     size_t inside_succ_cnt = 0;
  //     for (auto succ_id : bb->succ_list) {
  //       if (loop_info.body_id_set.count(succ_id)) {
  //         inside_succ_cnt++;
  //       }
  //     }
  //     if (inside_succ_cnt > 1) {
  //       return;
  //     }
  //   }
  // }
  
  // find loop index operand
  // 1. Appear in the phi instruction of the loop header, with one value coming
  // from inside the loop and the other is constant.
  // 2. The value coming from inside the loop is defined by add 1 / sub 1 instruciton.
  // 3. The value coming from inside the loop is used by an icmp instruction.
  auto header_bb = builder.context.get_basic_block(loop_info.header_id);

  OperandID loop_operand_id;
  OperandID icmp_operand_id; 
  int start_value;
  int end_value;
  bool do_loop_unrolling = false;

  for (auto instr = header_bb->head_instruction->next;
       instr != header_bb->tail_instruction && instr->is_phi();
       instr = instr->next) {
    
    std::optional<OperandID> maybe_loop_operand_id = std::nullopt;
    std::optional<OperandID> maybe_icmp_operand_id = std::nullopt;
    std::optional<int> maybe_start_value = std::nullopt;
    std::optional<int> maybe_end_value = std::nullopt;
    
    auto phi = instr->as<Phi>().value();
    if (phi.incoming_list.size() != 2) {
      continue;
      // break; // give up
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
      // if (maybe_alternate_id) {
      //   break; // give up
      // } else {
        continue;
      // }
    }

    auto alternative_id = maybe_alternate_id.value();
    auto start_id = maybe_start_id.value();

    auto alternative_operand = builder.context.get_operand(alternative_id);
    if (!alternative_operand->maybe_def_id.has_value()) {
      continue;
      // break; // give up
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
          auto constant = std::get<operand::ConstantPtr>(
            builder.context.get_operand(rhs_id)->kind
          );
          auto step = std::get<int>(constant->kind);
          // TODO: add more step cases
          if (step == 1) {
            maybe_loop_operand_id = phi.dst_id;
            auto start_constant = std::get<operand::ConstantPtr>(
              builder.context.get_operand(start_id)->kind
            );
            maybe_start_value = std::get<int>(start_constant->kind);
          }
        }
      }
    } 
    // else {
    //   break; // give up
    // }

    // check icmp instruction
    auto phi_dst_operand = builder.context.get_operand(phi.dst_id);
    for (auto use_inst_id : phi_dst_operand->use_id_list) {
      auto use_inst = builder.context.get_instruction(use_inst_id);
      if (use_inst->is_icmp()) {
        auto icmp = use_inst->as<ICmp>().value();
        // TODO: add more icmp cases
        if (icmp.cond == ICmpCond::Slt) {
          auto lhs_id = icmp.lhs_id;
          auto rhs_id = icmp.rhs_id;
          auto rhs = builder.context.get_operand(rhs_id);
          if (rhs->is_constant()) {
            auto constant = std::get<operand::ConstantPtr>(rhs->kind);
            auto end_value = std::get<int>(constant->kind);
            maybe_end_value = end_value - 1;
            maybe_icmp_operand_id = icmp.dst_id;
          }
        } else if (icmp.cond == ICmpCond::Sle) {
          auto lhs_id = icmp.lhs_id;
          auto rhs_id = icmp.rhs_id;
          auto rhs = builder.context.get_operand(rhs_id);
          if (rhs->is_constant()) {
            auto constant = std::get<operand::ConstantPtr>(rhs->kind);
            auto end_value = std::get<int>(constant->kind);
            maybe_end_value = end_value;
            maybe_icmp_operand_id = icmp.dst_id;
          }
        }
      }
    }

    // if all three conditions are satisfied, then we can do loop unrolling
    if (maybe_loop_operand_id.has_value() && maybe_start_value.has_value() && maybe_end_value.has_value()) {
      loop_operand_id = maybe_loop_operand_id.value();
      icmp_operand_id = maybe_icmp_operand_id.value();
      start_value = maybe_start_value.value();
      end_value = maybe_end_value.value();
      do_loop_unrolling = true;
      break;
    } 
    // else if (maybe_loop_operand_id || maybe_start_value || maybe_end_value) {
    //   // give up
    //   break;
    // }
  }

  if (do_loop_unrolling) {
    // debug
    std::cout << "loop unrolling %" << loop_operand_id << " from " << start_value << " to " << end_value << std::endl;
    // remove phi, icmp, br instructions
    auto phi_operand = builder.context.get_operand(loop_operand_id);
    auto phi_instruction = builder.context.get_instruction(phi_operand->maybe_def_id.value());
    auto icmp_operand = builder.context.get_operand(icmp_operand_id);
    auto icmp_instruction = builder.context.get_instruction(icmp_operand->maybe_def_id.value());
    builder.set_curr_basic_block(header_bb);

    if (icmp_operand->use_id_list.size() != 1) {
      throw std::runtime_error("icmp use id list size is not 1");
    }

    auto br_instruction_id = icmp_operand->use_id_list[0];
    auto br_instruction = builder.context.get_instruction(br_instruction_id);
    auto then_bb_id = br_instruction->as<CondBr>().value().then_block_id;
    auto then_bb = builder.context.get_basic_block(then_bb_id);
    auto else_bb_id = br_instruction->as<CondBr>().value().else_block_id;
    auto else_bb = builder.context.get_basic_block(else_bb_id);
    // std::cout << "then_bb_id: " << then_bb_id << std::endl;
    // std::cout << "else_bb_id: " << else_bb_id << std::endl;

    BasicBlockPtr next_first_bb = nullptr;
    BasicBlockPtr first_bb = nullptr;

    std::vector<BasicBlockPtr> new_bb_list;

    LoopUnrollingContext context = LoopUnrollingContext();
    LoopUnrollingContext last_context;
    context.loop_info = loop_info;

    // TODO: its a hack, need to fix
    for (int loop_index = start_value; loop_index <= end_value; loop_index++) {
      // std::cout << "loop_index: " << loop_index << std::endl;
      auto loop_index_operand = builder.fetch_constant_operand(
        builder.fetch_i32_type(), loop_index
      );
      auto loop_index_id = loop_index_operand;
      // context.operand_id_map[loop_operand_id] = loop_index_id;

      if (next_first_bb == nullptr) {
        // if it's the first iteration, fetch the next first basic block
        next_first_bb = builder.fetch_basic_block();
        new_bb_list.push_back(next_first_bb);
        first_bb = next_first_bb;
      }

      bool is_first = true;
      for (auto loopbody_bb_id : loop_info.body_id_set) {
        if (loopbody_bb_id == loop_info.header_id) {
          // simulate header basic block phi instruction
          for (auto curr_inst = header_bb->head_instruction->next;
               curr_inst != header_bb->tail_instruction && curr_inst->is_phi();
               curr_inst = curr_inst->next) {

            auto phi = curr_inst->as<Phi>().value();
            auto phi_operand_id = phi.dst_id;
            if (phi_operand_id == loop_operand_id) {
              context.operand_id_map[loop_operand_id] = loop_index_id;
            } else {
              if (phi.incoming_list.size() != 2) {
                throw std::runtime_error("loop unrolling: phi incoming list size is not 2");
              }

              OperandID outside_operand;
              OperandID inside_operand;
              for (auto [incoming_operand_id, incoming_bb_id] : phi.incoming_list) {
                if (loop_info.body_id_set.count(incoming_bb_id) == 0) {
                  // coming from outside the loop
                  outside_operand = incoming_operand_id;
                } else {
                  // coming from inside the loop
                  inside_operand = incoming_operand_id;
                }
              }

              if (is_first) {
                context.operand_id_map[phi_operand_id] = outside_operand;
              } else {
                context.operand_id_map[phi_operand_id] = last_context.operand_id_map[inside_operand];
              }
            }
          }
          continue;
        }
        BasicBlockPtr copied_loopbody_bb = nullptr;
        if (is_first) {
          copied_loopbody_bb = next_first_bb;
          is_first = false;
        } else {
          copied_loopbody_bb = builder.fetch_basic_block();
          new_bb_list.push_back(copied_loopbody_bb);
        }
        context.basic_block_id_map[loopbody_bb_id] = copied_loopbody_bb->id;
      }

      if (loop_index != end_value) {
        // if it's not the last iteration, fetch the next first basic block
        next_first_bb = builder.fetch_basic_block();
        new_bb_list.push_back(next_first_bb);
        context.basic_block_id_map[header_bb->id] = next_first_bb->id;
      } else {
        // if it's the last iteration, jump to the original else block
        context.basic_block_id_map[header_bb->id] = else_bb_id;
      }

      // Clone instructions
      for (auto loopbody_bb_id : loop_info.body_id_set) {
        if (loopbody_bb_id == loop_info.header_id) {
          continue;
        }
        auto loopbody_bb = builder.context.get_basic_block(loopbody_bb_id);
        auto copied_loopbody_bb = builder.context.get_basic_block(
          context.basic_block_id_map[loopbody_bb_id]
        );
        builder.set_curr_basic_block(copied_loopbody_bb);
        auto curr_instruction = loopbody_bb->head_instruction->next;
        while (curr_instruction != loopbody_bb->tail_instruction) {
          // std::cout << "curr_instruction: " << curr_instruction->to_string(builder.context) << std::endl;
          if (curr_instruction->is_ret()) {
            throw std::runtime_error("ret instruction in loop unrolling");
          }

          auto new_instruction =
            clone_instruction(curr_instruction, builder, context);
          if (new_instruction == nullptr) {
            curr_instruction = curr_instruction->next;
            continue;
          }
          
          builder.append_instruction(new_instruction);

          curr_instruction = curr_instruction->next;
        }
      }

      last_context = context;
      context = LoopUnrollingContext();
      context.loop_info = loop_info;
    }

    // put the new basic blocks into the function
    auto curr_bb = header_bb;
    for (auto new_bb : new_bb_list) {
      curr_bb->insert_next(new_bb);
      curr_bb = new_bb;
    }

    // retarget br instruction to the first basic block
    builder.set_curr_basic_block(header_bb);
    auto new_br_instruction = builder.fetch_br_instruction(first_bb->id);
    br_instruction->remove(builder.context);
    builder.append_instruction(new_br_instruction);

    // remove old loop body basic blocks
    for (auto loopbody_bb_id : loop_info.body_id_set) {
      if (loopbody_bb_id == loop_info.header_id) {
        continue;
      }
      auto loopbody_bb = builder.context.get_basic_block(loopbody_bb_id);
      loopbody_bb->succ_list.clear();
      loopbody_bb->pred_list.clear();
      loopbody_bb->use_id_list.clear();
      loopbody_bb->remove(builder.context);
    }
    // phi_instruction->remove(builder.context);
    icmp_instruction->remove(builder.context);

    builder.set_curr_basic_block(header_bb);
    // remove all phi instructions in the loop header basic block
    for (auto curr_inst = header_bb->head_instruction->next;
         curr_inst != header_bb->tail_instruction && curr_inst->is_phi();
         curr_inst = curr_inst->next) {
      // std::cout << "curr_inst: " << curr_inst->to_string(builder.context) << std::endl;
      curr_inst->remove(builder.context);
    }
  }
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
    auto def_inst_id = operand->maybe_def_id.value();
    auto def_inst = builder.context.get_instruction(def_inst_id);
    auto def_parent_bb_id = def_inst->parent_block_id;
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

        auto then_block_id =
          context.basic_block_id_map.at(condbr.then_block_id);
        auto else_block_id =
          context.basic_block_id_map.at(condbr.else_block_id);

        auto new_instruction = builder.fetch_condbr_instruction(
          cond_id, then_block_id, else_block_id
        );

        new_instruction->parent_block_id =
          context.basic_block_id_map.at(instruction->parent_block_id);

        return new_instruction;
      },
      [&](const Br& br) -> InstructionPtr {
        auto block_id = context.basic_block_id_map.at(br.block_id);

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
          incoming_list.push_back(std::make_tuple(
            clone_operand(incoming_operand_id, builder, context),
            context.basic_block_id_map.at(incoming_block_id)
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