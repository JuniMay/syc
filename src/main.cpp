
#include <fstream>
#include <iostream>
#include "frontend/driver.h"
#include "frontend/irgen.h"
#include "ir/builder.h"
#include "ir/instruction.h"
#include "backend/builder.h"
#include "ir/codegen.h"
#include "utils.h"

int main(int argc, char* argv[]) {

  using namespace syc;

  auto options = parse_args(argc, argv);

  frontend::Driver parse_driver(options.input_filename);

  int parse_success = parse_driver.parser->parse();

  auto& compunit = parse_driver.compunit;

  if (options.token_file.has_value()) {
    std::ofstream token_file(options.token_file.value());
    token_file << parse_driver.tokens;
  }

  if (options.ast_file.has_value()) {
    std::ofstream ast_file(options.ast_file.value());
    ast_file << compunit.to_string();
  }

  auto ir_builder = ir::Builder();

  irgen(compunit, ir_builder);

  if (options.ir_file.has_value()) {
    std::ofstream ir_file(options.ir_file.value());
    ir_file << ir_builder.context.to_string();
  }

  if (parse_success != 0) {
    std::cerr << "Parse failed." << std::endl;
    return 1;
  }

  auto asm_builder = backend::Builder();

  codegen(ir_builder.context, asm_builder);

  if (options.output_file.has_value()) {
    std::ofstream output_file(options.output_file.value());
    output_file << asm_builder.context.to_string();
  }

  return 0;

}
