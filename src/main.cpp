
#include <fstream>
#include <iostream>
#include "backend/builder.h"
#include "frontend/driver.h"
#include "frontend/irgen.h"
#include "ir/builder.h"
#include "ir/codegen.h"
#include "ir/instruction.h"
#include "passes/asm_dce.h"
#include "passes/asm_peephole.h"
#include "passes/asm_peephole_second.h"
#include "passes/ir_peephole.h"
#include "passes/linear_scan.h"
#include "passes/load_elim.h"
#include "passes/mem2reg.h"
#include "passes/phi_elim.h"
#include "passes/straighten.h"
#include "passes/unreach_elim.h"
#include "passes/unused_elim.h"
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
    ir::load_elim(ir_builder);
    ir::peephole(ir_builder);
    // Still problematic
    // ir::unreach_elim(ir_builder);
    ir::unused_elim(ir_builder);
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
    backend::peephole_second(asm_builder);
  }

  codegen_rest(ir_builder.context, asm_builder, codegen_context);

  if (options.output_file.has_value()) {
    std::ofstream output_file(options.output_file.value());
    output_file << asm_builder.context.to_string();
  }

  return 0;
}
