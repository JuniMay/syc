#include "passes/ir/gvn.h"

namespace syc {
namespace ir {

void gvn(Builder& builder, bool is_aggressive) {
  for (auto [function_name, function] : builder.context.function_table) {
    gvn_function(function, builder, is_aggressive);
  }
}

void gvn_function(FunctionPtr function, Builder& builder, bool is_aggressive) {
  auto curr_basic_block = function->head_basic_block->next;
  if (curr_basic_block == function->tail_basic_block) {
    return;
  }
  auto cfa_ctx = ControlFlowAnalysisContext();
  control_flow_analysis(function, builder.context, cfa_ctx);
  gvn_basic_block(function, curr_basic_block, builder, cfa_ctx, is_aggressive, 0);
}

using ValueNumMap = std::map<OperandID, OperandID>;
using BinaryExprMap = std::map<
  std::tuple<instruction::BinaryOp, std::variant<int, OperandID>, std::variant<int, OperandID>>,
  OperandID>;
using InvGetElementPtrExprMap = std::map<OperandID,
  std::tuple<std::string, OperandID, std::vector<std::variant<int, OperandID>>>>;
using GetElementPtrExprMap = std::map<std::tuple<std::string, OperandID, std::vector<std::variant<int, OperandID>>>,
  OperandID>;
using PhiExprMap = std::map<
  std::vector<std::tuple<OperandID, BasicBlockID>>,
  OperandID>;
using LoadExprMap = std::map<
  std::tuple<std::string, OperandID, std::vector<std::variant<int, OperandID>>>,
  OperandID>;
using StoreExprMap = std::map<
  std::tuple<std::string, OperandID, std::vector<std::variant<int, OperandID>>>,
  OperandID>;
using ICmpExprMap = std::map<
  std::tuple<syc::ir::instruction::ICmpCond, std::variant<int, OperandID>, std::variant<int, OperandID>>,
  OperandID>;
using FCmpExprMap = std::map<
  std::tuple<syc::ir::instruction::FCmpCond, std::variant<int, OperandID>, std::variant<int, OperandID>>,
  OperandID>;
using CastExprMap = std::map<
  std::tuple<syc::ir::instruction::CastOp, std::variant<int, OperandID>>,
  OperandID>;

ValueNumMap value_number_map;
PhiExprMap phi_expr_map;
BinaryExprMap binary_expr_map;
InvGetElementPtrExprMap inv_getelementptr_expr_map;
LoadExprMap load_expr_map;
StoreExprMap store_expr_map;
GetElementPtrExprMap getelementptr_expr_map;
ICmpExprMap icmp_expr_map;
FCmpExprMap fcmp_expr_map;
CastExprMap cast_expr_map;

void gvn_basic_block(FunctionPtr function, BasicBlockPtr basic_block, Builder& builder, ControlFlowAnalysisContext cfa_ctx, bool is_aggressive, size_t depth) {
  using namespace instruction;

  if (depth >= 500) {
    return;
  }

  builder.set_curr_basic_block(basic_block);
  
  // save old maps for restoring
  ValueNumMap value_number_map_copy = value_number_map;
  PhiExprMap phi_expr_map_copy = phi_expr_map;
  BinaryExprMap binary_expr_map_copy = binary_expr_map;
  InvGetElementPtrExprMap inv_getelementptr_expr_map_copy = inv_getelementptr_expr_map;
  LoadExprMap load_expr_map_copy = load_expr_map;
  StoreExprMap store_expr_map_copy = store_expr_map;
  GetElementPtrExprMap getelementptr_expr_map_copy = getelementptr_expr_map;
  ICmpExprMap icmp_expr_map_copy = icmp_expr_map;
  FCmpExprMap fcmp_expr_map_copy = fcmp_expr_map;
  CastExprMap cast_expr_map_copy = cast_expr_map;

  auto curr_instruction = basic_block->head_instruction->next;
  while (curr_instruction != basic_block->tail_instruction) {
    auto next_instruction = curr_instruction->next;
    if (curr_instruction->is_phi()) {
      auto curr_dst_id = curr_instruction->as<Phi>()->dst_id;
      auto instruction_key = curr_instruction->as<Phi>()->incoming_list;

      // replace operands with value number
      for (auto& [operand_id, basic_block_id] : instruction_key) {
        if (value_number_map.count(operand_id) > 0 && operand_id != value_number_map[operand_id]) {
          curr_instruction->replace_operand(
            operand_id, value_number_map[operand_id], builder.context
          );
        }
      }

      curr_dst_id = curr_instruction->as<Phi>()->dst_id;
      instruction_key = curr_instruction->as<Phi>()->incoming_list;

      if (phi_expr_map.count(instruction_key) > 0) {
        // redundant phi instruction
        value_number_map[curr_dst_id] = phi_expr_map[instruction_key];
        curr_instruction->remove(builder.context);
      } else {
        // for new phi instruction, the value number is the dst_id
        // value_number_map[curr_dst_id] = curr_dst_id;
        phi_expr_map[instruction_key] = curr_dst_id;
      }
  
    } else if (curr_instruction->is_binary()) {
      auto curr_lhs_id = curr_instruction->as<Binary>()->lhs_id;
      auto curr_rhs_id = curr_instruction->as<Binary>()->rhs_id;
      auto curr_dst_id = curr_instruction->as<Binary>()->dst_id;
      auto curr_op = curr_instruction->as<Binary>()->op;
      
      // replace operands with value number
      if (value_number_map.count(curr_lhs_id) > 0 && curr_lhs_id != value_number_map[curr_lhs_id]) {
        curr_instruction->replace_operand(
          curr_lhs_id, value_number_map[curr_lhs_id], builder.context
        );
      }
      if (value_number_map.count(curr_rhs_id) > 0 && curr_rhs_id != value_number_map[curr_rhs_id]) {
        curr_instruction->replace_operand(
          curr_rhs_id, value_number_map[curr_rhs_id], builder.context
        );
      }

      curr_lhs_id = curr_instruction->as<Binary>()->lhs_id;
      curr_rhs_id = curr_instruction->as<Binary>()->rhs_id;
      curr_dst_id = curr_instruction->as<Binary>()->dst_id;
      curr_op = curr_instruction->as<Binary>()->op;

      // generate instruction key
      std::variant<int, OperandID> lhs_id_copy;
      std::variant<int, OperandID> rhs_id_copy;
      if (builder.context.get_operand(curr_lhs_id)->is_constant()
          && builder.context.get_operand(curr_lhs_id)->is_int()) {
        auto constant = std::get<operand::ConstantPtr>(
          builder.context.get_operand(curr_lhs_id)->kind
        );
        auto constant_value = std::get<int>(constant->kind);
        lhs_id_copy = constant_value;
      } else {
        lhs_id_copy = curr_lhs_id;
      }
      if (builder.context.get_operand(curr_rhs_id)->is_constant()
          && builder.context.get_operand(curr_rhs_id)->is_int()) {
        auto constant = std::get<operand::ConstantPtr>(
          builder.context.get_operand(curr_rhs_id)->kind
        );
        auto constant_value = std::get<int>(constant->kind);
        rhs_id_copy = constant_value;
      } else {
        rhs_id_copy = curr_rhs_id;
      }

      // if op commutative, sort lhs and rhs to make sure the key is unique
      if (curr_op == BinaryOp::Add || curr_op == BinaryOp::Mul) {
        if (lhs_id_copy > rhs_id_copy) {
          std::swap(lhs_id_copy, rhs_id_copy);
        }
      }
      auto instruction_key = std::make_tuple(curr_op, lhs_id_copy, rhs_id_copy);

      if (binary_expr_map.count(instruction_key) > 0) {
        // redundant binary instruction
        value_number_map[curr_dst_id] = binary_expr_map[instruction_key];
        curr_instruction->remove(builder.context);
      } else {
        // for new binary instruction, the value number is the dst_id
        // value_number_map[curr_dst_id] = curr_dst_id;
        binary_expr_map[instruction_key] = curr_dst_id;
      }
    } else if (curr_instruction->is_icmp()) {
      auto curr_dst_id = curr_instruction->as<ICmp>()->dst_id;
      auto curr_cond = curr_instruction->as<ICmp>()->cond;
      auto curr_lhs_id = curr_instruction->as<ICmp>()->lhs_id;
      auto curr_rhs_id = curr_instruction->as<ICmp>()->rhs_id;
      auto curr_lhs_operand = builder.context.get_operand(curr_lhs_id);
      auto curr_rhs_operand = builder.context.get_operand(curr_rhs_id);

      // replace lhs and rhs with value number
      if (value_number_map.count(curr_lhs_id) > 0 && value_number_map[curr_lhs_id] != curr_lhs_id) {
        curr_instruction->replace_operand(curr_lhs_id, value_number_map[curr_lhs_id], builder.context);
      }
      if (value_number_map.count(curr_rhs_id) > 0 && value_number_map[curr_rhs_id] != curr_rhs_id) {
        curr_instruction->replace_operand(curr_rhs_id, value_number_map[curr_rhs_id], builder.context);
      }

      curr_dst_id = curr_instruction->as<ICmp>()->dst_id;
      curr_cond = curr_instruction->as<ICmp>()->cond;
      curr_lhs_id = curr_instruction->as<ICmp>()->lhs_id;
      curr_rhs_id = curr_instruction->as<ICmp>()->rhs_id;
      curr_lhs_operand = builder.context.get_operand(curr_lhs_id);
      curr_rhs_operand = builder.context.get_operand(curr_rhs_id);

      // generate instrction key
      std::variant<int, OperandID> lhs_id_copy;
      std::variant<int, OperandID> rhs_id_copy;
      if (curr_lhs_operand->is_constant() && curr_lhs_operand->is_int()) {
        auto constant = std::get<operand::ConstantPtr>(curr_lhs_operand->kind);
        auto constant_value = std::get<int>(constant->kind);
        lhs_id_copy = constant_value;
      } else {
        lhs_id_copy = curr_lhs_id;
      }
      if (curr_rhs_operand->is_constant() && curr_rhs_operand->is_int()) {
        auto constant = std::get<operand::ConstantPtr>(curr_rhs_operand->kind);
        auto constant_value = std::get<int>(constant->kind);
        rhs_id_copy = constant_value;
      } else {
        rhs_id_copy = curr_rhs_id;
      }

      // if op commutative, sort lhs and rhs to make sure the key is unique
      if (curr_cond == ICmpCond::Eq || curr_cond == ICmpCond::Ne) {
        if (lhs_id_copy > rhs_id_copy) {
          std::swap(lhs_id_copy, rhs_id_copy); // TODO: should i maintain sth?
        }
      }

      auto instruction_key = std::make_tuple(curr_cond, lhs_id_copy, rhs_id_copy);
      
      if (icmp_expr_map.count(instruction_key) > 0) {
        // redundant icmp instruction
        value_number_map[curr_dst_id] = icmp_expr_map[instruction_key];
        curr_instruction->remove(builder.context);
      } else {
        // for new icmp instruction, the value number is the dst_id
        // value_number_map[curr_dst_id] = curr_dst_id;
        icmp_expr_map[instruction_key] = curr_dst_id;
      }
    } 
    else if (curr_instruction->is_fcmp()) {
      auto curr_dst_id = curr_instruction->as<FCmp>()->dst_id;
      auto curr_cond = curr_instruction->as<FCmp>()->cond;
      auto curr_lhs_id = curr_instruction->as<FCmp>()->lhs_id;
      auto curr_rhs_id = curr_instruction->as<FCmp>()->rhs_id;
      auto curr_lhs_operand = builder.context.get_operand(curr_lhs_id);
      auto curr_rhs_operand = builder.context.get_operand(curr_rhs_id);

      // replace lhs and rhs with value number
      if (value_number_map.count(curr_lhs_id) > 0 && value_number_map[curr_lhs_id] != curr_lhs_id) {
        curr_instruction->replace_operand(curr_lhs_id, value_number_map[curr_lhs_id], builder.context);
      }
      if (value_number_map.count(curr_rhs_id) > 0 && value_number_map[curr_rhs_id] != curr_rhs_id) {
        curr_instruction->replace_operand(curr_rhs_id, value_number_map[curr_rhs_id], builder.context);
      }

      curr_dst_id = curr_instruction->as<FCmp>()->dst_id;
      curr_cond = curr_instruction->as<FCmp>()->cond;
      curr_lhs_id = curr_instruction->as<FCmp>()->lhs_id;
      curr_rhs_id = curr_instruction->as<FCmp>()->rhs_id;
      curr_lhs_operand = builder.context.get_operand(curr_lhs_id);
      curr_rhs_operand = builder.context.get_operand(curr_rhs_id);

      // generate instrction key
      std::variant<int, OperandID> lhs_id_copy;
      std::variant<int, OperandID> rhs_id_copy;
      if (curr_lhs_operand->is_constant() && curr_lhs_operand->is_int()) {
        auto constant = std::get<operand::ConstantPtr>(curr_lhs_operand->kind);
        auto constant_value = std::get<int>(constant->kind);
        lhs_id_copy = constant_value;
      } else {
        lhs_id_copy = curr_lhs_id;
      }
      if (curr_rhs_operand->is_constant() && curr_rhs_operand->is_int()) {
        auto constant = std::get<operand::ConstantPtr>(curr_rhs_operand->kind);
        auto constant_value = std::get<int>(constant->kind);
        rhs_id_copy = constant_value;
      } else {
        rhs_id_copy = curr_rhs_id;
      }

      // if op commutative, sort lhs and rhs to make sure the key is unique
      if (curr_cond == FCmpCond::Oeq || curr_cond == FCmpCond::One) {
        if (lhs_id_copy > rhs_id_copy) {
          std::swap(lhs_id_copy, rhs_id_copy); // TODO: should i maintain sth?
        }
      }

      auto instruction_key = std::make_tuple(curr_cond, lhs_id_copy, rhs_id_copy);
      
      if (fcmp_expr_map.count(instruction_key) > 0) {
        // redundant icmp instruction
        value_number_map[curr_dst_id] = fcmp_expr_map[instruction_key];
        curr_instruction->remove(builder.context);
      } else {
        // for new icmp instruction, the value number is the dst_id
        // value_number_map[curr_dst_id] = curr_dst_id;
        fcmp_expr_map[instruction_key] = curr_dst_id;
      }
    } 
    else if (curr_instruction->is_getelementptr()) {
      auto curr_dst_id = curr_instruction->as<GetElementPtr>()->dst_id;
      auto curr_basis_type = curr_instruction->as<GetElementPtr>()->basis_type;
      auto curr_ptr_id = curr_instruction->as<GetElementPtr>()->ptr_id;
      auto curr_index_id_list = curr_instruction->as<GetElementPtr>()->index_id_list;

      // replace operands with value number
      if (value_number_map.count(curr_ptr_id) > 0 && curr_ptr_id != value_number_map[curr_ptr_id]) {
        curr_instruction->replace_operand(
          curr_ptr_id, value_number_map[curr_ptr_id], builder.context
        );
      }
      for (auto curr_index_id : curr_index_id_list) {
        auto curr_index_operand = builder.context.get_operand(curr_index_id);
        if (!curr_index_operand->is_constant() && 
          value_number_map.count(curr_index_id) > 0 && 
          curr_index_id != value_number_map[curr_index_id]) {
          curr_instruction->replace_operand(
            curr_index_id, value_number_map[curr_index_id], builder.context
          );
        }
      }

      curr_dst_id = curr_instruction->as<GetElementPtr>()->dst_id;
      curr_basis_type = curr_instruction->as<GetElementPtr>()->basis_type;
      curr_ptr_id = curr_instruction->as<GetElementPtr>()->ptr_id;
      curr_index_id_list = curr_instruction->as<GetElementPtr>()->index_id_list;

      // generate instruction value
      std::vector<std::variant<int, OperandID>> index_id_list_copy;
      for (auto curr_index_id : curr_index_id_list) {
        if (builder.context.get_operand(curr_index_id)->is_constant()
            && builder.context.get_operand(curr_index_id)->is_int()) {
          auto constant = std::get<operand::ConstantPtr>(
            builder.context.get_operand(curr_index_id)->kind
          );
          auto constant_value = std::get<int>(constant->kind);
          index_id_list_copy.push_back(constant_value);
        } else {
          index_id_list_copy.push_back(curr_index_id);
        }
      }
      // std::cout << curr_basis_type->to_string() << std::endl;
      auto instruction_value = std::make_tuple(curr_basis_type->to_string(), curr_ptr_id, index_id_list_copy);
      inv_getelementptr_expr_map[curr_dst_id] = instruction_value;

      if (getelementptr_expr_map.count(instruction_value) > 0) {
        // redundant getelementptr instruction
        value_number_map[curr_dst_id] = getelementptr_expr_map[instruction_value];
        curr_instruction->remove(builder.context);
      } else {
        // for new getelementptr instruction, the value number is the dst_id
        // value_number_map[curr_dst_id] = curr_dst_id;
        getelementptr_expr_map[instruction_value] = curr_dst_id;
      }
      
    } 
    // else if (curr_instruction->is_cast()) {
    //   auto curr_dst_id = curr_instruction->as<Cast>()->dst_id;
    //   auto curr_op = curr_instruction->as<Cast>()->op;
    //   auto curr_src_id = curr_instruction->as<Cast>()->src_id;

    //   // replace operand with value number
    //   if (value_number_map.count(curr_src_id) > 0 && curr_src_id != value_number_map[curr_src_id]) {
    //     curr_instruction->replace_operand(
    //       curr_src_id, value_number_map[curr_src_id], builder.context
    //     );
    //   }

    //   curr_dst_id = curr_instruction->as<Cast>()->dst_id;
    //   curr_op = curr_instruction->as<Cast>()->op;
    //   curr_src_id = curr_instruction->as<Cast>()->src_id;

    //   // generate instruction key
    //   std::variant<int, OperandID> src_id_copy;
    //   if (builder.context.get_operand(curr_src_id)->is_constant()
    //       && builder.context.get_operand(curr_src_id)->is_int()) {
    //     auto constant = std::get<operand::ConstantPtr>(
    //       builder.context.get_operand(curr_src_id)->kind
    //     );
    //     auto constant_value = std::get<int>(constant->kind);
    //     src_id_copy = constant_value;
    //   } else {
    //     src_id_copy = curr_src_id;
    //   }
    //   auto instruction_key = std::make_tuple(curr_op, src_id_copy);

    //   if (cast_expr_map.count(instruction_key) > 0) {
    //     // redundant cast instruction
    //     value_number_map[curr_dst_id] = cast_expr_map[instruction_key];
    //     curr_instruction->remove(builder.context);
    //   } else {
    //     // for new cast instruction, the value number is the dst_id
    //     // value_number_map[curr_dst_id] = curr_dst_id;
    //     cast_expr_map[instruction_key] = curr_dst_id;
    //   }
    // }
    else if (is_aggressive && curr_instruction->is_store()) {
      auto curr_ptr_id = curr_instruction->as<Store>()->ptr_id;
      auto curr_value_id = curr_instruction->as<Store>()->value_id;

      // replace operands with value number
      if (value_number_map.count(curr_ptr_id) > 0 && curr_ptr_id != value_number_map[curr_ptr_id]) {
        curr_instruction->replace_operand(
          curr_ptr_id, value_number_map[curr_ptr_id], builder.context
        );
      }
      if (value_number_map.count(curr_value_id) > 0 && curr_value_id != value_number_map[curr_value_id]) {
        curr_instruction->replace_operand(
          curr_value_id, value_number_map[curr_value_id], builder.context
        );
      }

      curr_ptr_id = curr_instruction->as<Store>()->ptr_id;
      curr_value_id = curr_instruction->as<Store>()->value_id;
      auto curr_ptr_operand = builder.context.get_operand(curr_ptr_id);
      auto curr_value_operand = builder.context.get_operand(curr_value_id);
      
      if (curr_value_operand->is_constant()){
        // TODO: maybe we can optimize better??
        if (inv_getelementptr_expr_map.count(curr_ptr_id)) {
          // get related gep expr from inv_getelementptr_expr_map
          auto related_gep_expr = inv_getelementptr_expr_map[curr_ptr_id];
          store_expr_map[related_gep_expr] = curr_value_id;
        } else if (curr_ptr_operand->is_global()) {
          // global variable, treat as a gep expr with no index
          auto related_gep_expr = std::make_tuple(std::string(""), curr_ptr_id, std::vector<std::variant<int, OperandID>>{});
          store_expr_map[related_gep_expr] = curr_value_id;
        } else if (is_aggressive) {
          auto related_gep_expr = std::make_tuple(std::string(""), curr_ptr_id, std::vector<std::variant<int, OperandID>>{});
          store_expr_map[related_gep_expr] = curr_value_id;
        } else {
          throw std::runtime_error("store instruction without gep pointer");
        }
      }
    } 
    else if (is_aggressive && curr_instruction->is_load()) {
      auto curr_dst_id = curr_instruction->as<Load>()->dst_id;
      auto curr_ptr_id = curr_instruction->as<Load>()->ptr_id;

      // replace operands with value number
      if (value_number_map.count(curr_ptr_id) > 0 && curr_ptr_id != value_number_map[curr_ptr_id]) {
        curr_instruction->replace_operand(
          curr_ptr_id, value_number_map[curr_ptr_id], builder.context
        );
        curr_ptr_id = value_number_map[curr_ptr_id];
      }

      curr_dst_id = curr_instruction->as<Load>()->dst_id;
      curr_ptr_id = curr_instruction->as<Load>()->ptr_id;

      // get related gep expr from inv_getelementptr_expr_map
      std::tuple<std::string, syc::ir::OperandID, std::vector<std::variant<int, syc::ir::OperandID>>> related_gep_expr;
      auto curr_ptr_operand = builder.context.get_operand(curr_ptr_id);
      if (inv_getelementptr_expr_map.count(curr_ptr_id) > 0) {
        related_gep_expr = inv_getelementptr_expr_map[curr_ptr_id];
      } else if (curr_ptr_operand->is_global()) {
        // global variable, treat as a gep expr with no index
        related_gep_expr = std::make_tuple(std::string(""), curr_ptr_id, std::vector<std::variant<int, OperandID>>{});
      } else if (is_aggressive) {
        related_gep_expr = std::make_tuple(std::string(""), curr_ptr_id, std::vector<std::variant<int, OperandID>>{});
      } else {
        throw std::runtime_error("load instruction without gep pointer");
      } 

      // more aggressive optimization
      if (!is_aggressive) {
        auto related_gep_operand = builder.context.get_operand(std::get<1>(related_gep_expr));
        if (related_gep_operand->is_global()) {
          // std::cout << "global variable load" << related_gep_operand->to_string() << std::endl;
          curr_instruction = next_instruction;
          continue;
        }
      }

      if (store_expr_map.count(related_gep_expr) > 0) {
        // redundant load instruction
        // just treat it as a operand should be fine
        value_number_map[curr_dst_id] = store_expr_map[related_gep_expr];
        curr_instruction->remove(builder.context);
      } 
      // else if (load_expr_map.count(related_gep_expr) > 0) {
      //   // redundant load instruction
      //   value_number_map[curr_dst_id] = load_expr_map[related_gep_expr];
      //   curr_instruction->remove(builder.context);
      // } 
      else {
        auto curr_ptr_operand = builder.context.get_operand(curr_ptr_id);
        // for new load instruction, the value number is the dst_id
        // value_number_map[curr_dst_id] = curr_dst_id;
        load_expr_map[related_gep_expr] = curr_dst_id;
      }
    } 
    else if (curr_instruction->is_call()) {
      auto curr_arg_id_list = curr_instruction->as<Call>()->arg_id_list;
      for (auto curr_arg_id : curr_arg_id_list) {
        auto curr_arg_operand = builder.context.get_operand(curr_arg_id);
        if (curr_arg_operand->type->as<type::Pointer>().has_value()) {
          // std::cout << "call arg: " << curr_arg_operand->to_string() << std::endl;
          // remove all the load and store instructions related to the pointer from map
          auto curr_arg_ptr_id = curr_arg_operand->id;
          auto related_gep_expr = inv_getelementptr_expr_map[curr_arg_ptr_id];
          if (store_expr_map.count(related_gep_expr) > 0) {
            store_expr_map.erase(related_gep_expr);
          }
          if (load_expr_map.count(related_gep_expr) > 0) {
            load_expr_map.erase(related_gep_expr);
          }
        }
      }
      // std::cout << "call: " << curr_instruction->as<Call>()->function_name << std::endl;
      // remove all the load and store instructions related to the pointer of a global variable
      for (auto it = load_expr_map.begin(); it != load_expr_map.end(); ) {
        auto gep_expr = it->first;
        auto gep_ptr_id = std::get<1>(gep_expr);
        auto gep_ptr_operand = builder.context.get_operand(gep_ptr_id);
        if (gep_ptr_operand->is_global()) {
          // std::cout << "clear global load: " << gep_ptr_operand->to_string() << std::endl;
          it = load_expr_map.erase(it);
        } else {
          ++it;
        }
      }

      for (auto it = store_expr_map.begin(); it != store_expr_map.end(); ) {
        auto gep_expr = it->first;
        auto gep_ptr_id = std::get<1>(gep_expr);
        auto gep_ptr_operand = builder.context.get_operand(gep_ptr_id);
        if (gep_ptr_operand->is_global()) {
          // std::cout << "clear global store: " << gep_ptr_operand->to_string() << std::endl;
          it = store_expr_map.erase(it);
        } else {
          ++it;
        }
      }
    }

    curr_instruction = next_instruction;
  }

  // in case of irreplaced operands, replace them with value number
  for (auto kv : value_number_map) {
    auto from_operand_id = kv.first;
    auto to_operand_id = kv.second;
    // std::cout << "%" << from_operand_id << " -> %" << to_operand_id << std::endl;
    if (from_operand_id == to_operand_id) {
      // optimize for better performance
      continue;
    }
    auto from_operand = builder.context.get_operand(from_operand_id);
    auto to_operand = builder.context.get_operand(to_operand_id);
    auto use_id_list_copy = from_operand->use_id_list;
    for (auto use_id : use_id_list_copy) {
      auto use_instruction = builder.context.get_instruction(use_id);
      use_instruction->replace_operand(from_operand_id, to_operand_id, builder.context);
    }
  }

  // recursively gvn for children basic blocks
  for (auto child_bb : cfa_ctx.dom_tree[basic_block->id]) {
    gvn_basic_block(function, builder.context.get_basic_block(child_bb), builder, cfa_ctx, is_aggressive, depth + 1);
  }

  // restore maps
  value_number_map = value_number_map_copy;
  phi_expr_map = phi_expr_map_copy;
  binary_expr_map = binary_expr_map_copy;
  inv_getelementptr_expr_map = inv_getelementptr_expr_map_copy;
  load_expr_map = load_expr_map_copy;
  store_expr_map = store_expr_map_copy;
  getelementptr_expr_map = getelementptr_expr_map_copy;
  icmp_expr_map = icmp_expr_map_copy;
  fcmp_expr_map = fcmp_expr_map_copy;

}

}  // namespace ir
}  // namespace syc