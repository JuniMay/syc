
#include <iostream>
#include "ir/builder.h"
#include "ir/instruction.h"

#ifdef UNITTEST
#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"
#include "unittest/ir.h"
#endif

int main(int argc, char* argv[]) {
#ifdef UNITTEST
  int result = Catch::Session().run(argc, argv);
  return result;
#else

  auto builder = syc::ir::Builder();

  auto param_1 = builder.make_parameter_operand(builder.make_i32_type(), "a");
  auto param_2 = builder.make_parameter_operand(builder.make_float_type(), "b");

  auto global_zero = builder.make_global_operand(
    builder.make_i32_type(), "g_zero", false, true, {}
  );

  auto global_one = builder.make_global_operand(
    builder.make_i32_type(), "g_one", false, false,
    {
      builder.make_immediate_operand(builder.make_i32_type(), 1),
    }
  );

  builder.add_function(
    "add",
    {
      param_1,
      param_2,
    },
    builder.make_i32_type()
  );

  auto entry_block_id = builder.add_basic_block();

  auto gone_local_id = builder.make_arbitrary_operand(builder.make_i32_type());

  auto load_gone =
    builder.add_load_instruction(gone_local_id, global_one, std::nullopt);

  auto add_sum_operand_id =
    builder.make_arbitrary_operand(builder.make_i32_type());

  auto add_sum_instruction_id = builder.add_binary_instruction(
    syc::ir::instruction::BinaryOp::Add, add_sum_operand_id, param_1,
    gone_local_id
  );

  auto ret_instruction_id = builder.add_ret_instruction(add_sum_operand_id);

  auto ir_str = builder.context.to_string();

  std::cout << ir_str;

  return 0;

#endif
}
