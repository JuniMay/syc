
#include <fstream>
#include <iostream>
#include "backend__builder.h"
#include "frontend__driver.h"
#include "frontend__irgen.h"
#include "ir__builder.h"
#include "ir__codegen.h"
#include "ir__instruction.h"
#include "passes__asm_dce.h"
#include "passes__asm_peephole.h"
#include "passes__ir_peephole.h"
#include "passes__linear_scan.h"
#include "passes__mem2reg.h"
#include "passes__phi_elim.h"
#include "passes__unreach_elim.h"
#include "passes__straighten.h"
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

  if (options.optimization_level > 0) {
    ir::mem2reg(ir_builder);
    ir::straighten(ir_builder);
    ir::peephole(ir_builder);
    // Still problematic
    // ir::unreach_elim(ir_builder);
  }

  if (options.ir_file.has_value()) {
    std::ofstream ir_file(options.ir_file.value());
    ir_file << ir_builder.context.to_string();
  }

  if (parse_success != 0) {
    std::cerr << "Parse failed." << std::endl;
    return 1;
  }

  auto asm_builder = backend::Builder();
  auto codegen_context = CodegenContext();

  codegen(ir_builder.context, asm_builder, codegen_context);

  if (options.optimization_level > 0) {
    backend::peephole(asm_builder);
    backend::dce(asm_builder);
    // Still problematic
    backend::phi_elim(asm_builder);
  }

  codegen_rest(ir_builder.context, asm_builder, codegen_context);

  if (options.output_file.has_value()) {
    std::ofstream output_file(options.output_file.value());
    output_file << asm_builder.context.to_string();
  }

  return 0;
}
