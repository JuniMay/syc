
#include <fstream>
#include <iostream>
#include "backend__builder.h"
#include "frontend__driver.h"
#include "frontend__irgen.h"
#include "ir__builder.h"
#include "ir__codegen.h"
#include "ir__instruction.h"
#include "passes__asm__addr_simplification.h"
#include "passes__asm__dce.h"
#include "passes__asm__fast_divmod.h"
#include "passes__asm__instr_fuse.h"
#include "passes__asm__peephole.h"
#include "passes__asm__peephole_final.h"
#include "passes__asm__peephole_second.h"
#include "passes__asm__phi_elim.h"
#include "passes__ir__auto_inline.h"
#include "passes__ir__copyprop.h"
#include "passes__ir__dce.h"
#include "passes__ir__func_ret_opt.h"
#include "passes__ir__global2local.h"
#include "passes__ir__gvn.h"
#include "passes__ir__load_elim.h"
#include "passes__ir__loop_indvar_simplify.h"
#include "passes__ir__loop_invariant_motion.h"
#include "passes__ir__loop_unrolling.h"
#include "passes__ir__math_opt.h"
#include "passes__ir__mem2reg.h"
#include "passes__ir__peephole.h"
#include "passes__ir__purity_opt.h"
#include "passes__ir__straighten.h"
#include "passes__ir__strength_reduce.h"
#include "passes__ir__unreach_elim.h"
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

  bool aggressive_opt = options.aggressive_opt;

  if (options.optimization_level > 0) {
    ir::mem2reg(ir_builder);
    ir::func_ret_opt(ir_builder);
    ir::purity_opt(ir_builder);
    ir::auto_inline(ir_builder);
    ir::global2local(ir_builder);
    ir::mem2reg(ir_builder);
    ir::gvn(ir_builder, aggressive_opt);
    ir::load_elim(ir_builder);
    ir::loop_invariant_motion(ir_builder);
    ir::peephole(ir_builder);
    ir::unreach_elim(ir_builder);
    ir::straighten(ir_builder);
    for (int i = 0; i < 3; i++) {
      ir::peephole(ir_builder);
      ir::dce(ir_builder);
    }
    ir::math_opt(ir_builder);
    ir::dce(ir_builder);
    ir::copyprop(ir_builder);
    ir::peephole(ir_builder);
    ir::loop_unrolling(ir_builder);
    ir::copyprop(ir_builder);
    ir::gvn(ir_builder, aggressive_opt);
    ir::copyprop(ir_builder);
    ir::load_elim(ir_builder);
    ir::loop_invariant_motion(ir_builder);
    ir::peephole(ir_builder);
    ir::unreach_elim(ir_builder);
    ir::straighten(ir_builder);
    for (int i = 0; i < 3; i++) {
      ir::peephole(ir_builder);
      ir::dce(ir_builder);
    }
    ir::loop_indvar_simplify(ir_builder);
    ir::math_opt(ir_builder);
    ir::straighten(ir_builder);
    ir::peephole(ir_builder);
    ir::dce(ir_builder);
    ir::copyprop(ir_builder);
    ir::math_opt(ir_builder);
    ir::dce(ir_builder);
    ir::strength_reduce(ir_builder);
    ir::dce(ir_builder);
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

  backend::peephole(asm_builder);
  backend::dce(asm_builder);

  if (options.optimization_level > 0) {
    backend::phi_elim(asm_builder);
    for (int i = 0; i < 3; i++) {
      backend::peephole(asm_builder);
      backend::dce(asm_builder);
    }
    backend::peephole_second(asm_builder);
    backend::addr_simplification(asm_builder);
    backend::fast_divmod(asm_builder);
    if (aggressive_opt) {
      backend::instr_fuse(asm_builder);
      backend::dce(asm_builder);
    }
  }

  codegen_rest(ir_builder.context, asm_builder, codegen_context);

  backend::peephole_final(asm_builder);

  if (options.output_file.has_value()) {
    std::ofstream output_file(options.output_file.value());
    output_file << asm_builder.context.to_string();
  }

  return 0;
}
