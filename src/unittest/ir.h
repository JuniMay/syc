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

    builder.add_function("main", {}, builder.fetch_i32_type());

    auto bb_0 = builder.fetch_basic_block();
    builder.append_basic_block(bb_0);
    builder.set_curr_basic_block(bb_0);

    auto operand_0_id =
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0);
    auto operand_1_id =
      builder.fetch_constant_operand(builder.fetch_i32_type(), 1);

    auto dst_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_i32_type());
    auto add_instruction = builder.fetch_binary_instruction(
      BinaryOp::Add, dst_operand_id, operand_0_id, operand_1_id
    );

    auto ret_instruction = builder.fetch_ret_instruction(
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0)
    );
    builder.append_instruction(ret_instruction);

    ret_instruction->insert_prev(add_instruction);

    auto dst_operand_2_id =
      builder.fetch_arbitrary_operand(builder.fetch_i32_type());

    auto add_instruction_1 = builder.fetch_binary_instruction(
      BinaryOp::Add, dst_operand_2_id, dst_operand_id,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 2)
    );

    add_instruction->insert_next(add_instruction_1);

    std::cout << builder.context.to_string() << std::endl;
  }

  SECTION("Instruction Remove") {
    auto builder = Builder();

    builder.add_function("main", {}, builder.fetch_i32_type());

    auto bb_0 = builder.fetch_basic_block();
    builder.append_basic_block(bb_0);
    builder.set_curr_basic_block(bb_0);

    auto operand_0_id =
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0);
    auto operand_1_id =
      builder.fetch_constant_operand(builder.fetch_i32_type(), 1);

    auto dst_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_i32_type());
    auto add_instruction = builder.fetch_binary_instruction(
      BinaryOp::Add, dst_operand_id, operand_0_id, operand_1_id
    );

    auto ret_instruction = builder.fetch_ret_instruction(
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0)
    );
    builder.append_instruction(ret_instruction);

    ret_instruction->insert_prev(add_instruction);

    auto dst_operand_2_id =
      builder.fetch_arbitrary_operand(builder.fetch_i32_type());

    auto add_instruction_1 = builder.fetch_binary_instruction(
      BinaryOp::Add, dst_operand_2_id, dst_operand_id,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 2)
    );

    add_instruction->insert_next(add_instruction_1);

    add_instruction->remove();

    std::cout << builder.context.to_string() << std::endl;
  }
}

TEST_CASE("IR Basic Block", "[ir][basic_block]") {
  using namespace syc::ir;

  SECTION("Basic Block Insertion") {
    auto builder = Builder();

    auto param_a_id =
      builder.fetch_parameter_operand(builder.fetch_i32_type(), "a");

    builder.add_function(
      "control_flow_functions", {param_a_id}, builder.fetch_float_type()
    );

    auto entry_block = builder.fetch_basic_block();
    auto then_block = builder.fetch_basic_block();
    auto else_block = builder.fetch_basic_block();
    auto final_block = builder.fetch_basic_block();

    builder.set_curr_basic_block(entry_block);
    auto cond_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_i1_type());
    auto cond_instruction = builder.fetch_icmp_instruction(
      instruction::ICmpCond::Slt, cond_operand_id, param_a_id,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0)
    );
    builder.append_instruction(cond_instruction);
    auto condbr_instruction = builder.fetch_condbr_instruction(
      cond_operand_id, then_block->id, else_block->id
    );
    builder.append_instruction(condbr_instruction);

    builder.set_curr_basic_block(then_block);
    auto then_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto sitofp_instruction_1 = builder.fetch_cast_instruction(
      instruction::CastOp::SIToFP, then_operand_id, param_a_id
    );
    builder.append_instruction(sitofp_instruction_1);
    auto br_instruction_1 = builder.fetch_br_instruction(final_block->id);
    builder.append_instruction(br_instruction_1);

    builder.set_curr_basic_block(else_block);
    auto else_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto sitofp_instruction_2 = builder.fetch_cast_instruction(
      instruction::CastOp::SIToFP, else_operand_id, param_a_id
    );
    builder.append_instruction(sitofp_instruction_2);
    auto fmul_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto fmul_instruction = builder.fetch_binary_instruction(
      instruction::BinaryOp::FMul, fmul_operand_id, else_operand_id,
      builder.fetch_constant_operand(builder.fetch_float_type(), (float)2.0)
    );
    builder.append_instruction(fmul_instruction);

    auto br_instruction_2 = builder.fetch_br_instruction(final_block->id);
    builder.append_instruction(br_instruction_2);

    builder.set_curr_basic_block(final_block);
    auto phi_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto phi_instruction = builder.fetch_phi_instruction(
      phi_operand_id,
      {{then_operand_id, then_block->id}, {fmul_operand_id, else_block->id}}
    );
    builder.append_instruction(phi_instruction);
    auto ret_instruction = builder.fetch_ret_instruction(phi_operand_id);
    builder.append_instruction(ret_instruction);

    builder.append_basic_block(final_block);
    final_block->insert_prev(then_block);
    then_block->insert_prev(entry_block);
    entry_block->insert_next(else_block);

    std::cout << builder.context.to_string();
  }

  SECTION("Basic Block Remove") {
    auto builder = Builder();

    auto param_a_id =
      builder.fetch_parameter_operand(builder.fetch_i32_type(), "a");

    builder.add_function(
      "control_flow_functions", {param_a_id}, builder.fetch_float_type()
    );

    auto entry_block = builder.fetch_basic_block();
    auto then_block = builder.fetch_basic_block();
    auto else_block = builder.fetch_basic_block();
    auto final_block = builder.fetch_basic_block();

    builder.set_curr_basic_block(entry_block);
    auto cond_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_i1_type());
    auto cond_instruction = builder.fetch_icmp_instruction(
      instruction::ICmpCond::Slt, cond_operand_id, param_a_id,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0)
    );
    builder.append_instruction(cond_instruction);
    auto condbr_instruction = builder.fetch_condbr_instruction(
      cond_operand_id, then_block->id, else_block->id
    );
    builder.append_instruction(condbr_instruction);

    builder.set_curr_basic_block(then_block);
    auto then_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto sitofp_instruction_1 = builder.fetch_cast_instruction(
      instruction::CastOp::SIToFP, then_operand_id, param_a_id
    );
    builder.append_instruction(sitofp_instruction_1);
    auto br_instruction_1 = builder.fetch_br_instruction(final_block->id);
    builder.append_instruction(br_instruction_1);

    builder.set_curr_basic_block(else_block);
    auto else_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto sitofp_instruction_2 = builder.fetch_cast_instruction(
      instruction::CastOp::SIToFP, else_operand_id, param_a_id
    );
    builder.append_instruction(sitofp_instruction_2);
    auto fmul_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto fmul_instruction = builder.fetch_binary_instruction(
      instruction::BinaryOp::FMul, fmul_operand_id, else_operand_id,
      builder.fetch_constant_operand(builder.fetch_float_type(), (float)2.0)
    );
    builder.append_instruction(fmul_instruction);

    auto br_instruction_2 = builder.fetch_br_instruction(final_block->id);
    builder.append_instruction(br_instruction_2);

    builder.set_curr_basic_block(final_block);
    auto phi_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto phi_instruction = builder.fetch_phi_instruction(
      phi_operand_id,
      {{then_operand_id, then_block->id}, {fmul_operand_id, else_block->id}}
    );
    builder.append_instruction(phi_instruction);
    auto ret_instruction = builder.fetch_ret_instruction(phi_operand_id);
    builder.append_instruction(ret_instruction);

    builder.append_basic_block(final_block);
    final_block->insert_prev(then_block);
    then_block->insert_prev(entry_block);
    entry_block->insert_next(else_block);

    then_block->remove();
    else_block->remove();

    std::cout << builder.context.to_string();
  }
}

TEST_CASE("IR Builder", "[ir]") {
  // TODO: Better unittest for instruction/operand/basic_block

  using namespace syc::ir;

  SECTION("Global variable and constant definition") {
    auto builder = Builder();

    auto global_i32_0 = builder.fetch_global_operand(
      builder.fetch_i32_type(), "g_0", false,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0)
    );

    auto global_i32_1 = builder.fetch_global_operand(
      builder.fetch_i32_type(), "g_1", false,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 114514)
    );

    auto global_arr_multi_0 = builder.fetch_global_operand(
      builder.fetch_array_type(
        3, builder.fetch_array_type(5, builder.fetch_i32_type())
      ),
      "g_arr_multi_0", false,
      builder.fetch_constant_operand(
        builder.fetch_array_type(
          3, builder.fetch_array_type(5, builder.fetch_i32_type())
        ),
        std::vector<operand::ConstantPtr>({
          builder.fetch_constant(
            builder.fetch_array_type(5, builder.fetch_i32_type()),
            std::vector<operand::ConstantPtr>({
              builder.fetch_constant(builder.fetch_i32_type(), 1),
              builder.fetch_constant(builder.fetch_i32_type(), 2),
              builder.fetch_constant(builder.fetch_i32_type(), 3),
              builder.fetch_constant(builder.fetch_i32_type(), 4),
              builder.fetch_constant(builder.fetch_i32_type(), 5),
            })
          ),
          builder.fetch_constant(
            builder.fetch_array_type(5, builder.fetch_i32_type()),
            std::vector<operand::ConstantPtr>({
              builder.fetch_constant(builder.fetch_i32_type(), 6),
              builder.fetch_constant(builder.fetch_i32_type(), 7),
              builder.fetch_constant(builder.fetch_i32_type(), 8),
              builder.fetch_constant(builder.fetch_i32_type(), 9),
              builder.fetch_constant(builder.fetch_i32_type(), 10),
            })
          ),
          builder.fetch_constant(
            builder.fetch_array_type(5, builder.fetch_i32_type()),
            std::vector<operand::ConstantPtr>({
              builder.fetch_constant(builder.fetch_i32_type(), 11),
              builder.fetch_constant(builder.fetch_i32_type(), 12),
              builder.fetch_constant(builder.fetch_i32_type(), 13),
              builder.fetch_constant(builder.fetch_i32_type(), 14),
              builder.fetch_constant(builder.fetch_i32_type(), 15),
            })
          ),
        })
      )
    );

    auto global_arr_1 = builder.fetch_global_operand(
      builder.fetch_array_type(3, builder.fetch_i32_type()), "g_arr_1", false,
      builder.fetch_constant_operand(
        builder.fetch_array_type(3, builder.fetch_i32_type()),
        operand::Zeroinitializer{}
      )
    );

    auto global_float_0 = builder.fetch_global_operand(
      builder.fetch_float_type(), "g_float_0", false,
      builder.fetch_constant_operand(builder.fetch_float_type(), (float)0.0)
    );

    auto global_float_1 = builder.fetch_global_operand(
      builder.fetch_float_type(), "g_float_1", false,

      builder.fetch_constant_operand(builder.fetch_float_type(), (float)3.14)

    );
    auto global_float_2 = builder.fetch_global_operand(
      builder.fetch_float_type(), "g_float_2", false,

      builder.fetch_constant_operand(
        builder.fetch_float_type(), (float)1067869798
      )
    );

    auto constant_i32_0 = builder.fetch_global_operand(
      builder.fetch_i32_type(), "c_0", true,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0)
    );
    auto constant_i32_1 = builder.fetch_global_operand(
      builder.fetch_i32_type(), "c_1", true,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 1919810)
    );

    auto constant_arr_0 = builder.fetch_global_operand(
      builder.fetch_array_type(3, builder.fetch_i32_type()), "c_arr_0", true,
      builder.fetch_constant_operand(
        builder.fetch_array_type(3, builder.fetch_i32_type()),
        std::vector<operand::ConstantPtr>({
          builder.fetch_constant(builder.fetch_i32_type(), 4),
          builder.fetch_constant(builder.fetch_i32_type(), 5),
          builder.fetch_constant(builder.fetch_i32_type(), 6),
        })
      )
    );

    auto constant_arr_1 = builder.fetch_global_operand(
      builder.fetch_array_type(3, builder.fetch_i32_type()), "c_arr_1", true,
      builder.fetch_constant_operand(
        builder.fetch_array_type(3, builder.fetch_i32_type()),
        operand::Zeroinitializer{}
      )
    );

    builder.add_function("dummy", {}, builder.fetch_void_type());

    auto main_function_entry_block = builder.fetch_basic_block();
    builder.append_basic_block(main_function_entry_block);
    builder.set_curr_basic_block(main_function_entry_block);

    auto ret_instruction = builder.fetch_ret_instruction();
    builder.append_instruction(ret_instruction);

    std::cout << builder.context.to_string();

    CHECK(true);
  }

  SECTION("Operand def and use") {
    auto builder = Builder();

    auto param_1_id =
      builder.fetch_parameter_operand(builder.fetch_i32_type(), "param_1");
    auto param_2_id =
      builder.fetch_parameter_operand(builder.fetch_i32_type(), "param_2");
    auto param_3_id =
      builder.fetch_parameter_operand(builder.fetch_float_type(), "param_3");

    builder.add_function(
      "test_fn",
      {
        param_1_id,
        param_2_id,
        param_3_id,
      },
      builder.fetch_i32_type()
    );

    auto test_fn_entry_block = builder.fetch_basic_block();
    builder.append_basic_block(test_fn_entry_block);
    builder.set_curr_basic_block(test_fn_entry_block);

    auto operand_0_id =
      builder.fetch_arbitrary_operand(builder.fetch_i32_type());
    auto instruction_0 = builder.fetch_binary_instruction(
      instruction::BinaryOp::Add, operand_0_id, param_1_id, param_2_id
    );
    builder.append_instruction(instruction_0);

    auto operand_1_id =
      builder.fetch_arbitrary_operand(builder.fetch_i32_type());
    auto instruction_1 = builder.fetch_cast_instruction(
      instruction::CastOp::FPToSI, operand_1_id, param_3_id
    );
    builder.append_instruction(instruction_1);

    auto operand_2_id =
      builder.fetch_arbitrary_operand(builder.fetch_i32_type());
    auto instruction_2 = builder.fetch_binary_instruction(
      instruction::BinaryOp::Sub, operand_2_id, operand_0_id, operand_1_id
    );
    builder.append_instruction(instruction_2);

    auto ret_instruction = builder.fetch_ret_instruction(operand_2_id);
    builder.append_instruction(ret_instruction);

    REQUIRE(ret_instruction->parent_block_id == test_fn_entry_block->id);

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
      builder.fetch_parameter_operand(builder.fetch_i32_type(), "a");

    builder.add_function(
      "control_flow_functions", {param_a_id}, builder.fetch_float_type()
    );

    auto entry_block = builder.fetch_basic_block();
    auto then_block = builder.fetch_basic_block();
    auto else_block = builder.fetch_basic_block();
    auto final_block = builder.fetch_basic_block();

    builder.append_basic_block(entry_block);
    builder.append_basic_block(then_block);
    builder.append_basic_block(else_block);
    builder.append_basic_block(final_block);

    builder.set_curr_basic_block(entry_block);

    auto cond_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_i1_type());

    auto cond_instruction = builder.fetch_icmp_instruction(
      instruction::ICmpCond::Slt, cond_operand_id, param_a_id,
      builder.fetch_constant_operand(builder.fetch_i32_type(), 0)
    );
    builder.append_instruction(cond_instruction);

    auto condbr_instruction = builder.fetch_condbr_instruction(
      cond_operand_id, then_block->id, else_block->id
    );
    builder.append_instruction(condbr_instruction);

    builder.set_curr_basic_block(then_block);

    auto then_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto sitofp_instruction_1 = builder.fetch_cast_instruction(
      instruction::CastOp::SIToFP, then_operand_id, param_a_id
    );
    builder.append_instruction(sitofp_instruction_1);

    auto br_instruction_1 = builder.fetch_br_instruction(final_block->id);
    builder.append_instruction(br_instruction_1);

    builder.set_curr_basic_block(else_block);

    auto else_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto sitofp_instruction_2 = builder.fetch_cast_instruction(
      instruction::CastOp::SIToFP, else_operand_id, param_a_id
    );
    builder.append_instruction(sitofp_instruction_2);

    auto fmul_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto fmul_instruction = builder.fetch_binary_instruction(
      instruction::BinaryOp::FMul, fmul_operand_id, else_operand_id,
      builder.fetch_constant_operand(builder.fetch_float_type(), (float)2.0)
    );
    builder.append_instruction(fmul_instruction);

    auto br_instruction_2 = builder.fetch_br_instruction(final_block->id);
    builder.append_instruction(br_instruction_2);

    builder.set_curr_basic_block(final_block);

    auto phi_operand_id =
      builder.fetch_arbitrary_operand(builder.fetch_float_type());
    auto phi_instruction = builder.fetch_phi_instruction(
      phi_operand_id,
      {{then_operand_id, then_block->id}, {fmul_operand_id, else_block->id}}
    );
    builder.append_instruction(phi_instruction);

    auto ret_instruction = builder.fetch_ret_instruction(phi_operand_id);
    builder.append_instruction(ret_instruction);

    std::cout << builder.context.to_string();

    REQUIRE(cond_instruction->parent_block_id == entry_block->id);
    REQUIRE(condbr_instruction->parent_block_id == entry_block->id);
    REQUIRE(sitofp_instruction_1->parent_block_id == then_block->id);
    REQUIRE(br_instruction_1->parent_block_id == then_block->id);
    REQUIRE(sitofp_instruction_2->parent_block_id == else_block->id);
    REQUIRE(fmul_instruction->parent_block_id == else_block->id);
    REQUIRE(br_instruction_2->parent_block_id == else_block->id);
    REQUIRE(phi_instruction->parent_block_id == final_block->id);
    REQUIRE(ret_instruction->parent_block_id == final_block->id);

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