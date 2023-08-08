#include "passes/ir/ptr_opt.h"
#include "ir/basic_block.h"
#include "ir/function.h"
#include "ir/instruction.h"
#include "ir/operand.h"
#include "passes/ir/control_flow_analysis.h"

namespace syc {
namespace ir {

void fill_data(operand::ConstantPtr constant, std::vector<uint8_t>& data) {
  auto type = constant->type;
  size_t size = type->get_size() / 8;

  std::visit(
    overloaded{
      [&](int a) {
        auto value = reinterpret_cast<uint32_t&>(a);
        std::cout << value << std::endl;
        for (size_t i = 0; i < size; i++) {
          auto byte = (uint8_t)(value >> (i * 8));
          data.push_back(byte);
        }
      },
      [&](float a) {
        auto value = reinterpret_cast<uint32_t&>(a);
        std::cout << value << std::endl;
        for (size_t i = 0; i < size; i++) {
          auto byte = (uint8_t)(value >> (i * 8));
          data.push_back(byte);
        }
      },
      [&](const std::vector<operand::ConstantPtr>& constant_list) {
        for (auto constant : constant_list) {
          fill_data(constant, data);
        }
      },
      [&](operand::Zeroinitializer zeroinitializer) {
        for (size_t i = 0; i < size; i++) {
          data.push_back(0);
        }
      },
      [&](const auto&) {
        for (size_t i = 0; i < size; i++) {
          data.push_back(0);
        }
      },
    },
    constant->kind
  );
}

void ptr_opt(Builder& builder) {
  using namespace instruction;

  auto ptr_opt_ctx = PtrOptContext();

  // Global
  for (auto operand_id : builder.context.global_list) {
    auto operand = builder.context.get_operand(operand_id);
    size_t size = operand->type->get_size() / 8;

    auto data = std::vector<uint8_t>(0, 0);

    auto global = std::get<operand::Global>(operand->kind);

    auto init = builder.context.get_operand(global.init);

    auto constant = std::get<operand::ConstantPtr>(init->kind);

    fill_data(constant, data);

    auto mem_obj = MemObj{operand_id, std::move(data)};
    auto ptr_obj = PtrObj{operand_id, 0};

    // DEBUG mem_obj data
    //  std::cout << "mem_obj data: ";
    //  for (auto byte : mem_obj.data) {
    //    std::cout << (int)byte << " ";
    //  }

    ptr_opt_ctx.base_ptr_set.insert(operand_id);
    ptr_opt_ctx.ptr_obj_map[operand_id] = std::move(ptr_obj);
    ptr_opt_ctx.mem_obj_map[operand_id] = std::move(mem_obj);
  }

  std::unordered_set<OperandID> stored_ptr_set;

  // Local
  for (auto [function_name, function] : builder.context.function_table) {
    for (auto bb = function->head_basic_block->next;
         bb != function->tail_basic_block; bb = bb->next) {
      for (auto instr = bb->head_instruction->next;
           instr != bb->tail_instruction; instr = instr->next) {
        if (instr->is_alloca()) {
          auto alloca = instr->as<Alloca>().value();
          auto ptr_id = alloca.dst_id;
          auto allocated_type = alloca.allocated_type;

          auto ptr_obj = PtrObj{ptr_id, 0};
          auto mem_obj = MemObj{
            ptr_id, std::vector<uint8_t>(allocated_type->get_size() / 8, 0)};

          ptr_opt_ctx.base_ptr_set.insert(ptr_id);
          ptr_opt_ctx.ptr_obj_map[ptr_id] = std::move(ptr_obj);
          ptr_opt_ctx.mem_obj_map[ptr_id] = std::move(mem_obj);
        } else if (instr->is_store()) {
          auto store_ptr_id = instr->as<Store>()->ptr_id;

          auto maybe_ptr_obj = get_ptr(store_ptr_id, builder, ptr_opt_ctx);
          if (!maybe_ptr_obj.has_value()) {
            continue;
          }
          // TODO: More precise
          stored_ptr_set.insert(maybe_ptr_obj->base_ptr_id);
        } else if (instr->is_call()) {
          // TODO: More precise interprocedural analysis
          auto call = instr->as<Call>().value();
          for (auto arg_id : call.arg_id_list) {
            auto arg = builder.context.get_operand(arg_id);
            if (!arg->get_type()->as<type::Pointer>().has_value()) {
              continue;
            }
            auto maybe_ptr_obj = get_ptr(arg_id, builder, ptr_opt_ctx);
            if (!maybe_ptr_obj.has_value()) {
              continue;
            }
            auto ptr_obj = maybe_ptr_obj.value();
            ptr_opt_ctx.uncertain_mem_obj_set.insert(ptr_obj.base_ptr_id);
          }
        }
      }
    }
  }

  for (auto [function_name, function] : builder.context.function_table) {
    for (auto bb = function->head_basic_block->next;
         bb != function->tail_basic_block; bb = bb->next) {
      for (auto instr = bb->head_instruction->next;
           instr != bb->tail_instruction; instr = instr->next) {
        if (instr->is_load()) {
          auto load = instr->as<Load>().value();
          auto maybe_ptr_obj = get_ptr(load.ptr_id, builder, ptr_opt_ctx);

          if (!maybe_ptr_obj.has_value()) {
            continue;
          }

          auto ptr_obj = maybe_ptr_obj.value();

          const auto& mem_obj = ptr_opt_ctx.mem_obj_map[ptr_obj.base_ptr_id];

          if (ptr_obj.offset.has_value() && 
              !stored_ptr_set.count(ptr_obj.base_ptr_id) && 
              !ptr_opt_ctx.uncertain_mem_obj_set.count(ptr_obj.base_ptr_id)) {
            auto offset = ptr_obj.offset.value();
            auto dst = builder.context.get_operand(load.dst_id);
            auto load_size = dst->get_type()->get_size() / 8;

            uint32_t value = 0;
            for (size_t i = 0; i < load_size; i++) {
              value |= mem_obj.data[offset + i] << (i * 8);
            }

            if (dst->get_type()->as<type::Float>()) {
              float f_constant = reinterpret_cast<float&>(value);
              auto constant_id =
                builder.fetch_constant_operand(dst->type, f_constant);
              // replace use
              auto use_id_list_copy = dst->use_id_list;
              for (auto use_id : use_id_list_copy) {
                auto use_instr = builder.context.get_instruction(use_id);
                use_instr->replace_operand(
                  dst->id, constant_id, builder.context
                );
              }
            } else {
              int i_constant = static_cast<int>(value);
              auto constant_id =
                builder.fetch_constant_operand(dst->type, i_constant);
              // replace use
              auto use_id_list_copy = dst->use_id_list;
              for (auto use_id : use_id_list_copy) {
                auto use_instr = builder.context.get_instruction(use_id);
                use_instr->replace_operand(
                  dst->id, constant_id, builder.context
                );
              }
            }
          }
        }
      }
    }
  }
}

std::optional<PtrObj>
get_ptr(OperandID& operand_id, Builder& builder, PtrOptContext& ptr_opt_ctx) {
  using namespace instruction;

  if (ptr_opt_ctx.ptr_obj_map.count(operand_id)) {
    return ptr_opt_ctx.ptr_obj_map[operand_id];
  }
  auto operand = builder.context.get_operand(operand_id);
  // std::cout << operand->to_string() << std::endl;

  if (!operand->maybe_def_id.has_value()) {
    // Parameters
    auto ptr_obj = PtrObj{operand_id, 0};
    auto mem_obj = MemObj{operand_id, std::vector<uint8_t>()};
    ptr_opt_ctx.base_ptr_set.insert(operand_id);
    ptr_opt_ctx.ptr_obj_map[operand_id] = ptr_obj;
    ptr_opt_ctx.mem_obj_map[operand_id] = mem_obj;
    ptr_opt_ctx.uncertain_mem_obj_set.insert(operand_id);
    return ptr_obj;
  }

  auto def_instr =
    builder.context.get_instruction(operand->maybe_def_id.value());

  auto maybe_gep = def_instr->as<GetElementPtr>();
  auto maybe_cast = def_instr->as<Cast>();
  auto maybe_phi = def_instr->as<Phi>();

  if (maybe_gep.has_value()) {
    auto gep = maybe_gep.value();
    auto basis_type = gep.basis_type;

    auto maybe_ptr_obj = get_ptr(gep.ptr_id, builder, ptr_opt_ctx);
    if (!maybe_ptr_obj.has_value()) {
      return std::nullopt;
    }
    auto ptr_obj = maybe_ptr_obj.value();

    if (!ptr_obj.offset.has_value()) {
      auto new_ptr_obj = PtrObj{ptr_obj.base_ptr_id, std::nullopt};
      ptr_opt_ctx.ptr_obj_map[operand_id] = new_ptr_obj;
      ptr_opt_ctx.uncertain_mem_obj_set.insert(ptr_obj.base_ptr_id);
      return new_ptr_obj;
    }

    size_t offset = 0;

    for (auto index_id : gep.index_id_list) {
      auto index = builder.context.get_operand(index_id);

      if (!index->is_constant()) {
        auto new_ptr_obj = PtrObj{ptr_obj.base_ptr_id, std::nullopt};
        ptr_opt_ctx.ptr_obj_map[operand_id] = new_ptr_obj;
        ptr_opt_ctx.uncertain_mem_obj_set.insert(ptr_obj.base_ptr_id);
        return new_ptr_obj;
      }

      auto constant = std::get<operand::ConstantPtr>(index->kind);

      auto index_value = std::get<int>(constant->kind);

      offset += basis_type->get_size() / 8 * index_value;

      if (!basis_type->as<type::Array>().has_value()) {
        break;
      } else {
        basis_type = basis_type->as<type::Array>().value().element_type;
      }
    }

    auto new_ptr_obj =
      PtrObj{ptr_obj.base_ptr_id, ptr_obj.offset.value() + offset};
    ptr_opt_ctx.ptr_obj_map[operand_id] = new_ptr_obj;
    return new_ptr_obj;
  } else if (maybe_cast.has_value() && maybe_cast->op == CastOp::BitCast) {
    auto cast = maybe_cast.value();
    auto maybe_ptr_obj = get_ptr(cast.src_id, builder, ptr_opt_ctx);
    if (!maybe_ptr_obj.has_value()) {
      return std::nullopt;
    }
    auto ptr_obj = maybe_ptr_obj.value();
    auto new_ptr_obj = PtrObj{ptr_obj.base_ptr_id, ptr_obj.offset};
    ptr_opt_ctx.ptr_obj_map[operand_id] = new_ptr_obj;
    return new_ptr_obj;
  } else if (maybe_phi.has_value()) {
    return std::nullopt;
  } else {
    std::cerr << "ptr_opt: get_ptr: unknown def instruction" << std::endl
              << def_instr->to_string(builder.context) << std::endl;

    return std::nullopt;
  }
}

}  // namespace ir
}  // namespace syc