#include <iostream>

#include "common.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/instruction.h"
#include "ir/operand.h"

#include "catch2/catch.hpp"

TEST_CASE("IR Builder", "[ir]") {
  // TODO: Better unittest for instruction/operand/basic_block

  using namespace syc::ir;

  SECTION("Global variable and constant definition") {
    auto builder = Builder();

    auto global_i32_0 = builder.make_global_operand(
      builder.make_i32_type(), "g_0", false, true, {}
    );

    auto global_i32_1 = builder.make_global_operand(
      builder.make_i32_type(), "g_1", false, false,
      {
        builder.make_immediate_operand(builder.make_i32_type(), 114514),
      }
    );

    auto global_arr_0 = builder.make_global_operand(
      builder.make_array_type(3, builder.make_i32_type()), "g_arr_0", false,
      false,
      {
        builder.make_immediate_operand(builder.make_i32_type(), 1),
        builder.make_immediate_operand(builder.make_i32_type(), 2),
        builder.make_immediate_operand(builder.make_i32_type(), 3),
      }
    );

    auto global_arr_1 = builder.make_global_operand(
      builder.make_array_type(3, builder.make_i32_type()), "g_arr_1", false,
      true, {}
    );

    auto global_float_0 = builder.make_global_operand(
      builder.make_float_type(), "g_float_0", false, true, {}
    );

    auto global_float_1 = builder.make_global_operand(
      builder.make_float_type(), "g_float_1", false, false,
      {
        builder.make_immediate_operand(builder.make_float_type(), (float)3.14),
      }
    );
    auto global_float_2 = builder.make_global_operand(
      builder.make_float_type(), "g_float_2", false, false,
      {
        builder.make_immediate_operand(
          builder.make_float_type(), (float)1067869798
        ),
      }
    );

    auto constant_i32_0 = builder.make_global_operand(
      builder.make_i32_type(), "c_0", true, true, {}
    );
    auto constant_i32_1 = builder.make_global_operand(
      builder.make_i32_type(), "c_1", true, false,
      {
        builder.make_immediate_operand(builder.make_i32_type(), 1919810),
      }
    );

    auto constant_arr_0 = builder.make_global_operand(
      builder.make_array_type(3, builder.make_i32_type()), "c_arr_0", true,
      false,
      {
        builder.make_immediate_operand(builder.make_i32_type(), 4),
        builder.make_immediate_operand(builder.make_i32_type(), 5),
        builder.make_immediate_operand(builder.make_i32_type(), 6),
      }
    );

    auto constant_arr_1 = builder.make_global_operand(
      builder.make_array_type(3, builder.make_i32_type()), "c_arr_1", true,
      true, {}
    );

    builder.add_function("dummy", {}, builder.make_void_type());

    auto main_function_entry_block_id = builder.add_basic_block();

    auto ret_instruction_id = builder.add_ret_instruction();

    std::cout << builder.context.to_string();

    CHECK(true);
  }

  SECTION("Operand def and use") {
    auto builder = Builder();

    auto param_1_id =
      builder.make_parameter_operand(builder.make_i32_type(), "param_1");
    auto param_2_id =
      builder.make_parameter_operand(builder.make_i32_type(), "param_2");
    auto param_3_id =
      builder.make_parameter_operand(builder.make_float_type(), "param_3");

    builder.add_function(
      "test_fn",
      {
        param_1_id,
        param_2_id,
        param_3_id,
      },
      builder.make_i32_type()
    );

    auto test_fn_entry_block_id = builder.add_basic_block();

    auto operand_0_id = builder.make_arbitrary_operand(builder.make_i32_type());
    auto instruction_0_id = builder.add_binary_instruction(
      instruction::BinaryOp::Add, operand_0_id, param_1_id, param_2_id
    );

    auto operand_1_id = builder.make_arbitrary_operand(builder.make_i32_type());
    auto instruction_1_id = builder.add_cast_instruction(
      instruction::CastOp::FPToSI, operand_1_id, param_3_id
    );
    auto operand_2_id = builder.make_arbitrary_operand(builder.make_i32_type());
    auto instruction_2_id = builder.add_binary_instruction(
      instruction::BinaryOp::Sub, operand_2_id, operand_0_id, operand_1_id
    );

    auto ret_instr_id = builder.add_ret_instruction(operand_2_id);

    REQUIRE(
      builder.context.get_operand(operand_0_id)->def_id == instruction_0_id
    );

    auto param_1 = builder.context.get_operand(param_1_id);
    auto param_2 = builder.context.get_operand(param_2_id);
    auto param_3 = builder.context.get_operand(param_3_id);

    REQUIRE(
      std::find(
        param_1->use_ids.begin(), param_1->use_ids.end(), instruction_0_id
      ) != param_1->use_ids.end()
    );
    REQUIRE(
      std::find(
        param_2->use_ids.begin(), param_2->use_ids.end(), instruction_0_id
      ) != param_2->use_ids.end()
    );
    REQUIRE(
      std::find(
        param_3->use_ids.begin(), param_3->use_ids.end(), instruction_1_id
      ) != param_3->use_ids.end()
    );

    auto operand_1 = builder.context.get_operand(operand_1_id);
    REQUIRE(operand_1->def_id == instruction_1_id);
    REQUIRE(
      std::find(
        operand_1->use_ids.begin(), operand_1->use_ids.end(), instruction_2_id
      ) != operand_1->use_ids.end()
    );

    std::cout << builder.context.to_string();
  }

  SECTION("Control flow instructions") {
    auto builder = Builder();

    auto param_a_id =
      builder.make_parameter_operand(builder.make_i32_type(), "a");

    builder.add_function(
      "control_flow_functions", {param_a_id}, builder.make_float_type()
    );

    auto entry_block_id = builder.add_basic_block();
    auto then_block_id = builder.add_basic_block();
    auto else_block_id = builder.add_basic_block();
    auto final_block_id = builder.add_basic_block();

    builder.switch_basic_block(entry_block_id);

    auto cond_operand_id =
      builder.make_arbitrary_operand(builder.make_i1_type());

    auto cond_instruction_id = builder.add_icmp_instruction(
      instruction::ICmpCond::Slt, cond_operand_id, param_a_id,
      builder.make_immediate_operand(builder.make_i32_type(), 0)
    );

    auto condbr_instruction_id = builder.add_condbr_instruction(
      cond_operand_id, then_block_id, else_block_id
    );

    builder.switch_basic_block(then_block_id);

    auto then_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto sitofp_instruction_id_1 = builder.add_cast_instruction(
      instruction::CastOp::SIToFP, then_operand_id, param_a_id
    );

    auto br_instruction_id_1 = builder.add_br_instruction(final_block_id);

    builder.switch_basic_block(else_block_id);

    auto else_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto sitofp_instruction_id_2 = builder.add_cast_instruction(
      instruction::CastOp::SIToFP, else_operand_id, param_a_id
    );

    auto fmul_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto fmul_instruction_id = builder.add_binary_instruction(
      instruction::BinaryOp::FMul, fmul_operand_id, else_operand_id,
      builder.make_immediate_operand(builder.make_float_type(), (float)2.0)
    );

    auto br_instruction_id_2 = builder.add_br_instruction(final_block_id);

    builder.switch_basic_block(final_block_id);

    auto phi_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto phi_instruction_id = builder.add_phi_instruction(
      phi_operand_id,
      {{then_operand_id, then_block_id}, {fmul_operand_id, else_block_id}}
    );

    auto ret_instruction_id = builder.add_ret_instruction(phi_operand_id);

    std::cout << builder.context.to_string();

    auto entry_block = builder.context.get_basic_block(entry_block_id);
    auto then_block = builder.context.get_basic_block(then_block_id);
    auto else_block = builder.context.get_basic_block(else_block_id);
    auto final_block = builder.context.get_basic_block(final_block_id);

    REQUIRE(
      std::find(
        then_block->use_ids.begin(), then_block->use_ids.end(),
        condbr_instruction_id
      ) != then_block->use_ids.end()
    );

    REQUIRE(
      std::find(
        else_block->use_ids.begin(), else_block->use_ids.end(),
        condbr_instruction_id
      ) != else_block->use_ids.end()
    );

    REQUIRE(
      std::find(
        then_block->use_ids.begin(), then_block->use_ids.end(),
        phi_instruction_id
      ) != then_block->use_ids.end()
    );

    REQUIRE(
      std::find(
        else_block->use_ids.begin(), else_block->use_ids.end(),
        phi_instruction_id
      ) != else_block->use_ids.end()
    );

    REQUIRE(
      std::find(
        final_block->use_ids.begin(), final_block->use_ids.end(),
        br_instruction_id_1
      ) != final_block->use_ids.end()
    );

    REQUIRE(
      std::find(
        final_block->use_ids.begin(), final_block->use_ids.end(),
        br_instruction_id_2
      ) != final_block->use_ids.end()
    );
  }
}