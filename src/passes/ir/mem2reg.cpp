#include "passes/ir/mem2reg.h"
#include "ir/basic_block.h"
#include "ir/context.h"
#include "ir/instruction.h"
#include "ir/operand.h"

namespace syc {
namespace ir {

void mem2reg(Builder& builder) {
  for (auto [function_name, function] : builder.context.function_table) {
    if (function->is_declare) {
      continue;
    }
    auto cfa_ctx = ControlFlowAnalysisContext();
    auto mem2reg_ctx = Mem2regContext();
    control_flow_analysis(function, builder.context, cfa_ctx);

    // Prepare variable set
    auto entry_bb = function->head_basic_block->next;
    auto curr_instr = entry_bb->head_instruction->next;
    while (curr_instr != entry_bb->tail_instruction && curr_instr->is_alloca()
    ) {
      auto next_instr = curr_instr->next;

      auto alloca = std::get<instruction::Alloca>(curr_instr->kind);

      auto allocated_type = alloca.allocated_type;

      bool convert = allocated_type->as<type::Integer>().has_value() ||
                     allocated_type->as<type::Float>().has_value() ||
                     allocated_type->as<type::Pointer>().has_value();

      // Only convert integer and float
      if (convert) {
        // Add the memory operand to variable set
        mem2reg_ctx.variable_set.insert(alloca.dst_id);
        // Initialize the maps
        mem2reg_ctx.def_map[alloca.dst_id] = {};
        mem2reg_ctx.inserted_map[alloca.dst_id] = {};
        mem2reg_ctx.worklist_map[alloca.dst_id] = {};
        mem2reg_ctx.rename_stack_map[alloca.dst_id] = {};
        // Remove alloca instruction
        curr_instr->remove(builder.context);
      }

      curr_instr = next_instr;
    }
    // Insert phi instruction
    insert_phi(function, builder, cfa_ctx, mem2reg_ctx);
    // Rename variables
    rename(entry_bb, builder, cfa_ctx, mem2reg_ctx);
  }
}

void insert_phi(
  FunctionPtr function,
  Builder& builder,
  ControlFlowAnalysisContext& cfa_ctx,
  Mem2regContext& mem2reg_ctx
) {
  // Add all definition into corresponding worklist
  auto curr_bb = function->head_basic_block->next;
  while (curr_bb != function->tail_basic_block) {
    auto curr_instr = curr_bb->head_instruction->next;

    while (curr_instr != curr_bb->tail_instruction) {
      if (curr_instr->is_store()) {
        auto store = std::get<instruction::Store>(curr_instr->kind);
        auto ptr_id = store.ptr_id;
        // For a to-be-converted variable, every block of its definition is only
        // added into the worklist once
        if (mem2reg_ctx.variable_set.count(ptr_id) && !mem2reg_ctx.def_map[ptr_id].count(curr_bb->id)) {
          mem2reg_ctx.def_map[ptr_id].insert(curr_bb->id);
          mem2reg_ctx.worklist_map[ptr_id].push(curr_bb->id);
        }
      }
      curr_instr = curr_instr->next;
    }
    curr_bb = curr_bb->next;
  }

  for (auto& [operand_id, worklist] : mem2reg_ctx.worklist_map) {
    // Memory operand. The type is pointer.
    auto operand = builder.context.get_operand(operand_id);

    while (!worklist.empty()) {
      auto bb_id = worklist.front();
      worklist.pop();

      for (auto df_id : cfa_ctx.dominance_frontier_map[bb_id]) {
        if (mem2reg_ctx.inserted_map[operand_id].count(df_id)) {
          // if block is already inserted, just skip
          continue;
        }
        // insert phi to the start of dominance frontier block
        auto df_bb = builder.context.get_basic_block(df_id);
        builder.set_curr_basic_block(df_bb);
        auto dst_id = builder.fetch_arbitrary_operand(
          operand->type->as<type::Pointer>().value().value_type
        );
        auto phi_instr = builder.fetch_phi_instruction(dst_id, {});
        builder.prepend_instruction_to_curr_basic_block(phi_instr);

        // Add the phi instruction to the map
        mem2reg_ctx.phi_map[phi_instr->id] = operand_id;

        // Mark as inserted
        mem2reg_ctx.inserted_map[operand_id].insert(df_id);

        if (!mem2reg_ctx.def_map[operand_id].count(df_id)) {
          mem2reg_ctx.worklist_map[operand_id].push(df_id);
        }
      }
    }
  }
}

void rename(
  BasicBlockPtr basic_block,
  Builder& builder,
  ControlFlowAnalysisContext& cfa_ctx,
  Mem2regContext& mem2reg_ctx
) {
  std::map<OperandID, size_t> def_cnt_map;

  auto curr_instr = basic_block->head_instruction->next;

  while (curr_instr != basic_block->tail_instruction) {
    auto next_instr = curr_instr->next;

    if (curr_instr->is_phi()) {
      auto phi = std::get<instruction::Phi>(curr_instr->kind);
      auto variable_id = mem2reg_ctx.phi_map[curr_instr->id];
      auto dst_id = phi.dst_id;

      def_cnt_map[variable_id] = 1;

      mem2reg_ctx.rename_stack_map[variable_id].push(dst_id);

    } else if (curr_instr->is_load()) {
      auto load = std::get<instruction::Load>(curr_instr->kind);

      if (mem2reg_ctx.variable_set.count(load.ptr_id)) {
        auto operand_id = mem2reg_ctx.rename_stack_map[load.ptr_id].top();

        auto load_dst = builder.context.get_operand(load.dst_id);

        auto use_id_list_copy = load_dst->use_id_list;

        for (auto instr_id : use_id_list_copy) {
          auto instr = builder.context.get_instruction(instr_id);
          instr->replace_operand(load.dst_id, operand_id, builder.context);
        }

        curr_instr->remove(builder.context);
      }

    } else if (curr_instr->is_store()) {
      auto store = std::get<instruction::Store>(curr_instr->kind);

      if (mem2reg_ctx.variable_set.count(store.ptr_id)) {
        // push the value to the stack
        mem2reg_ctx.rename_stack_map[store.ptr_id].push(store.value_id);

        if (!def_cnt_map.count(store.ptr_id)) {
          def_cnt_map[store.ptr_id] = 1;
        } else {
          def_cnt_map[store.ptr_id] += 1;
        }

        curr_instr->remove(builder.context);
      }
    } else if (curr_instr->is_br()) {
      break;
    }

    curr_instr = next_instr;
  }

  // Edit the phi instruction
  for (auto succ_id : basic_block->succ_list) {
    auto succ_bb = builder.context.get_basic_block(succ_id);
    auto curr_instr = succ_bb->head_instruction->next;

    while (curr_instr->is_phi()) {
      if (mem2reg_ctx.rename_stack_map[mem2reg_ctx.phi_map[curr_instr->id]].size() > 0) {
        curr_instr->add_phi_operand(
          mem2reg_ctx.rename_stack_map[mem2reg_ctx.phi_map[curr_instr->id]].top(
          ),
          basic_block->id, builder.context
        );
      } else {
        auto phi = std::get<instruction::Phi>(curr_instr->kind);
        auto phi_type = builder.context.get_operand(phi.dst_id)->type;

        if (phi_type->as<type::Integer>().has_value()) {
          curr_instr->add_phi_operand(
            builder.fetch_constant_operand(phi_type, (int)0), basic_block->id,
            builder.context
          );
        } else if (phi_type->as<type::Float>().has_value()) {
          curr_instr->add_phi_operand(
            builder.fetch_constant_operand(phi_type, (float)0), basic_block->id,
            builder.context
          );
        }
      }
      curr_instr = curr_instr->next;
    }
  }

  // DFS
  for (auto child_id : cfa_ctx.dom_tree[basic_block->id]) {
    rename(
      builder.context.get_basic_block(child_id), builder, cfa_ctx, mem2reg_ctx
    );
  }

  // Restore the stack
  for (auto [operand_id, def_cnt] : def_cnt_map) {
    for (size_t i = 0; i < def_cnt; i++) {
      mem2reg_ctx.rename_stack_map[operand_id].pop();
    }
  }
}

}  // namespace ir
}  // namespace syc