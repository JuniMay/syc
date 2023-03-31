#include <iostream>

#include "common.h"
#include "ir/basic_block.h"
#include "ir/builder.h"
#include "ir/instruction.h"
#include "ir/operand.h"

#include "catch2/catch.hpp"

TEST_CASE("IR Instruction", "[ir][instruction]") {
  using namespace syc::ir;
  using namespace syc::ir::instruction;

  SECTION("Instruction Insertion") {
    auto builder = Builder();

    builder.add_function("main", {}, builder.make_i32_type());

    auto bb_0_id = builder.add_basic_block();

    auto operand_0_id =
      builder.make_immediate_operand(builder.make_i32_type(), 0);
    auto operand_1_id =
      builder.make_immediate_operand(builder.make_i32_type(), 1);

    auto dst_operand_id =
      builder.make_arbitrary_operand(builder.make_i32_type());
    auto add_instruction = builder.make_binary_instruction(
      BinaryOp::Add, dst_operand_id, operand_0_id, operand_1_id
    );

    auto ret_instruction = builder.make_ret_instruction(
      builder.make_immediate_operand(builder.make_i32_type(), 0)
    );
    builder.add_instruction(ret_instruction);

    ret_instruction->insert_prev(add_instruction);

    auto dst_operand_2_id =
      builder.make_arbitrary_operand(builder.make_i32_type());

    auto add_instruction_1 = builder.make_binary_instruction(
      BinaryOp::Add, dst_operand_2_id, dst_operand_id,
      builder.make_immediate_operand(builder.make_i32_type(), 2)
    );

    add_instruction->insert_next(add_instruction_1);

    std::cout << builder.context.to_string() << std::endl;
  }
}

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

    auto ret_instruction = builder.make_ret_instruction();
    builder.add_instruction(ret_instruction);

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
    auto instruction_0 = builder.make_binary_instruction(
      instruction::BinaryOp::Add, operand_0_id, param_1_id, param_2_id
    );
    builder.add_instruction(instruction_0);

    auto operand_1_id = builder.make_arbitrary_operand(builder.make_i32_type());
    auto instruction_1 = builder.make_cast_instruction(
      instruction::CastOp::FPToSI, operand_1_id, param_3_id
    );
    builder.add_instruction(instruction_1);

    auto operand_2_id = builder.make_arbitrary_operand(builder.make_i32_type());
    auto instruction_2 = builder.make_binary_instruction(
      instruction::BinaryOp::Sub, operand_2_id, operand_0_id, operand_1_id
    );
    builder.add_instruction(instruction_2);

    auto ret_instruction = builder.make_ret_instruction(operand_2_id);
    builder.add_instruction(ret_instruction);

    REQUIRE(ret_instruction->parent_block_id == test_fn_entry_block_id);

    REQUIRE(
      builder.context.get_operand(operand_0_id)->def_id == instruction_0->id
    );

    auto param_1 = builder.context.get_operand(param_1_id);
    auto param_2 = builder.context.get_operand(param_2_id);
    auto param_3 = builder.context.get_operand(param_3_id);

    REQUIRE(
      std::find(
        param_1->use_id_list.begin(), param_1->use_id_list.end(),
        instruction_0->id
      ) != param_1->use_id_list.end()
    );
    REQUIRE(
      std::find(
        param_2->use_id_list.begin(), param_2->use_id_list.end(),
        instruction_0->id
      ) != param_2->use_id_list.end()
    );
    REQUIRE(
      std::find(
        param_3->use_id_list.begin(), param_3->use_id_list.end(),
        instruction_1->id
      ) != param_3->use_id_list.end()
    );

    auto operand_1 = builder.context.get_operand(operand_1_id);
    REQUIRE(operand_1->def_id == instruction_1->id);
    REQUIRE(
      std::find(
        operand_1->use_id_list.begin(), operand_1->use_id_list.end(),
        instruction_2->id
      ) != operand_1->use_id_list.end()
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

    auto cond_instruction = builder.make_icmp_instruction(
      instruction::ICmpCond::Slt, cond_operand_id, param_a_id,
      builder.make_immediate_operand(builder.make_i32_type(), 0)
    );
    builder.add_instruction(cond_instruction);

    auto condbr_instruction = builder.make_condbr_instruction(
      cond_operand_id, then_block_id, else_block_id
    );
    builder.add_instruction(condbr_instruction);

    builder.switch_basic_block(then_block_id);

    auto then_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto sitofp_instruction_1 = builder.make_cast_instruction(
      instruction::CastOp::SIToFP, then_operand_id, param_a_id
    );
    builder.add_instruction(sitofp_instruction_1);

    auto br_instruction_1 = builder.make_br_instruction(final_block_id);
    builder.add_instruction(br_instruction_1);

    builder.switch_basic_block(else_block_id);

    auto else_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto sitofp_instruction_2 = builder.make_cast_instruction(
      instruction::CastOp::SIToFP, else_operand_id, param_a_id
    );
    builder.add_instruction(sitofp_instruction_2);

    auto fmul_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto fmul_instruction = builder.make_binary_instruction(
      instruction::BinaryOp::FMul, fmul_operand_id, else_operand_id,
      builder.make_immediate_operand(builder.make_float_type(), (float)2.0)
    );
    builder.add_instruction(fmul_instruction);

    auto br_instruction_2 = builder.make_br_instruction(final_block_id);
    builder.add_instruction(br_instruction_2);

    builder.switch_basic_block(final_block_id);

    auto phi_operand_id =
      builder.make_arbitrary_operand(builder.make_float_type());
    auto phi_instruction = builder.make_phi_instruction(
      phi_operand_id,
      {{then_operand_id, then_block_id}, {fmul_operand_id, else_block_id}}
    );
    builder.add_instruction(phi_instruction);

    auto ret_instruction = builder.make_ret_instruction(phi_operand_id);
    builder.add_instruction(ret_instruction);

    std::cout << builder.context.to_string();

    REQUIRE(cond_instruction->parent_block_id == entry_block_id);
    REQUIRE(condbr_instruction->parent_block_id == entry_block_id);
    REQUIRE(sitofp_instruction_1->parent_block_id == then_block_id);
    REQUIRE(br_instruction_1->parent_block_id == then_block_id);
    REQUIRE(sitofp_instruction_2->parent_block_id == else_block_id);
    REQUIRE(fmul_instruction->parent_block_id == else_block_id);
    REQUIRE(br_instruction_2->parent_block_id == else_block_id);
    REQUIRE(phi_instruction->parent_block_id == final_block_id);
    REQUIRE(ret_instruction->parent_block_id == final_block_id);

    auto entry_block = builder.context.get_basic_block(entry_block_id);
    auto then_block = builder.context.get_basic_block(then_block_id);
    auto else_block = builder.context.get_basic_block(else_block_id);
    auto final_block = builder.context.get_basic_block(final_block_id);

    REQUIRE(
      std::find(
        then_block->use_id_list.begin(), then_block->use_id_list.end(),
        condbr_instruction->id
      ) != then_block->use_id_list.end()
    );

    REQUIRE(
      std::find(
        else_block->use_id_list.begin(), else_block->use_id_list.end(),
        condbr_instruction->id
      ) != else_block->use_id_list.end()
    );

    REQUIRE(
      std::find(
        then_block->use_id_list.begin(), then_block->use_id_list.end(),
        phi_instruction->id
      ) != then_block->use_id_list.end()
    );

    REQUIRE(
      std::find(
        else_block->use_id_list.begin(), else_block->use_id_list.end(),
        phi_instruction->id
      ) != else_block->use_id_list.end()
    );

    REQUIRE(
      std::find(
        final_block->use_id_list.begin(), final_block->use_id_list.end(),
        br_instruction_1->id
      ) != final_block->use_id_list.end()
    );

    REQUIRE(
      std::find(
        final_block->use_id_list.begin(), final_block->use_id_list.end(),
        br_instruction_2->id
      ) != final_block->use_id_list.end()
    );
  }
}