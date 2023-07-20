#include "passes__auto_inline.h"

namespace syc {
namespace ir {

void auto_inline(Builder& builder) {
  std::vector<std::string> to_be_removed;
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    // Judge if the function is recursive
    bool is_rec = false;
    for (auto call_id : function->caller_id_list) {
      auto call_instruction = builder.context.get_instruction(call_id);
      auto parent_basic_block =
        builder.context.get_basic_block(call_instruction->parent_block_id);
      if (function_name == parent_basic_block->parent_function_name) {
        is_rec = true;
        break;
      }
    }
    if (is_rec) {
      continue;
    }
    auto caller_id_list_copy = function->caller_id_list;
    for (auto call_id : caller_id_list_copy) {
      auto call_instruction = builder.context.get_instruction(call_id);
      auto_inline_instruction(call_instruction, builder);
    }
    if (function->caller_id_list.size() == 0 && function->name != "main") {
      to_be_removed.push_back(function_name);
    }
  }
  for (auto function_name : to_be_removed) {
    builder.context.function_table.erase(function_name);
  }
}

void auto_inline_instruction(InstructionPtr instruction, Builder& builder) {
  if (!instruction->is_call()) {
    return;
  }

  auto call = std::get<instruction::Call>(instruction->kind);

  auto function = builder.context.get_function(call.function_name);

  auto parent_basic_block =
    builder.context.get_basic_block(instruction->parent_block_id);

  auto parent_function_name = parent_basic_block->parent_function_name;

  builder.switch_function(parent_function_name);

  auto context = AutoInlineContext();

  // Map parameters
  for (size_t i = 0; i < function->parameter_id_list.size(); i++) {
    auto parameter_id = function->parameter_id_list[i];
    auto argument_id = call.arg_id_list[i];
    context.operand_id_map[parameter_id] = argument_id;
  }

  // Map blocks
  auto inline_basic_block =
    builder.context.get_basic_block(instruction->parent_block_id);

  inline_basic_block->split(instruction, builder);

  auto curr_basic_block = function->head_basic_block->next;

  context.basic_block_id_map[curr_basic_block->id] = inline_basic_block->id;
  curr_basic_block = curr_basic_block->next;

  while (curr_basic_block != function->tail_basic_block) {
    auto new_basic_block = builder.fetch_basic_block();
    context.basic_block_id_map[curr_basic_block->id] = new_basic_block->id;

    inline_basic_block->insert_next(new_basic_block);
    inline_basic_block = new_basic_block;
    curr_basic_block = curr_basic_block->next;
  }

  // Clone instructions
  curr_basic_block = function->head_basic_block->next;
  while (curr_basic_block != function->tail_basic_block) {
    auto inline_basic_block = builder.context.get_basic_block(
      context.basic_block_id_map[curr_basic_block->id]
    );

    builder.set_curr_basic_block(inline_basic_block);

    auto curr_instruction = curr_basic_block->head_instruction->next;
    while (curr_instruction != curr_basic_block->tail_instruction) {
      if (curr_instruction->is_ret()) {
        break;
      }

      auto new_instruction =
        clone_instruction(curr_instruction, builder, context);
      if (new_instruction == nullptr) {
        curr_instruction = curr_instruction->next;
        continue;
      }

      if (new_instruction->is_alloca()) {
        builder.prepend_instruction_to_curr_function(new_instruction);
      } else {
        builder.append_instruction(new_instruction);
      }

      curr_instruction = curr_instruction->next;
    }

    curr_basic_block = curr_basic_block->next;
  }

  // Map return value
  auto exit_basic_block = function->tail_basic_block->prev.lock();
  auto ret_instruction = exit_basic_block->tail_instruction->prev.lock();

  if (!ret_instruction->is_ret()) {
    std::cerr << "Error: Return instruction not found in the end of function "
              << function->name << std::endl;
  }
  auto ret = std::get<instruction::Ret>(ret_instruction->kind);
  if (ret.maybe_value_id.has_value() && call.maybe_dst_id.has_value()) {
    OperandID ret_value_id = ret.maybe_value_id.value();
    if (context.operand_id_map.count(ret_value_id)) {
      ret_value_id = context.operand_id_map.at(ret_value_id);
    }
    auto dst = builder.context.get_operand(call.maybe_dst_id.value());
    auto use_id_list_copy = dst->use_id_list;
    for (auto use_id : use_id_list_copy) {
      auto use_instruction = builder.context.get_instruction(use_id);

      use_instruction->replace_operand(
        call.maybe_dst_id.value(), ret_value_id, builder.context
      );
    }
  }

  // Add terminators
  builder.context.get_function(parent_function_name)->add_terminators(builder);

  instruction->remove(builder.context);
}

OperandID clone_operand(
  OperandID operand_id,
  Builder& builder,
  AutoInlineContext& context
) {
  if (context.operand_id_map.count(operand_id)) {
    return context.operand_id_map[operand_id];
  }
  auto operand = builder.context.get_operand(operand_id);
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
  AutoInlineContext& context
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