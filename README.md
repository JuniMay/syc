# SysY Compiler

## Getting Started

### Development

1. `sudo apt install flex cmake clang`
2. install bison 3.8
   1. `wget http://ftp.gnu.org/gnu/bison/bison-3.8.tar.gz`
   2. `tar -zxvf bison-3.8.tar.gz`
   3. `cd bison-3.8`
   4. `./configure`
   5. `make`
   6. `sudo make install`
   7. `sudo ln -s /usr/local/bin/bison /usr/bin/bison`
3. Generate the parser and lexer using `yacc_gen.sh` under scripts directory.
4. Build using cmake.

### Tests

The machanism of the contest is compiling everything altogether. So we need to flatten the source code.

```shell
python3 tests/execute.py
```

This will generate the `syc` executable in the current directory.

## Todo List

- [x] Try to use temporary register for temporary immediate loading.
  - t0, t1, t2 is used in linear scan.
  - t3 is used in codegen for temporary immediate loading, pseudo load/store and comparison.
  - t4 is used in argument passing (temporarily store the new sp).
  - t5, t6 and all temporary floating-point registers are not used yet.
- [x] Correct linear scan.
- [x] Fix `hidden_functional/29_long_line` (register allocation)
- [x] Fix `hidden_functional/30_many_dimensions` (segmentation fault)
- [x] Fix `hidden_functional/38_light2d`
  - Rounding mode of `fcvt` instruction.
- [ ] Constant folding in AST
  - Note that logical and/or cannot be folded because of short-circuit evaluation.
  - [x] Constant folding for most expressions
  - Fix ` functional/82_long_func.sy` (constant array slices with variable indices - incorrectly returns  `true `when using `is_const`)
- [ ] Unreachable elimination.
  - [ ] Fix `functional/51_short_circuit`
  - [ ] Fix `functional/75_max_flow`
  - [ ] Fix `functional/82_long_func`
- [x] Mem2reg.
- [ ] Phi elimination.
  - Done in ASM.
  - [ ] Fix `hidden_functional/19_search` and `hidden_functional/35_math`
- [ ] Constant propagation
- [x] `memset` for local arrays.
- [ ] Peephole optimization for IR and assembly.
  - `performance/instruction-combining`
  - IR: combine `load` and `store` with the same address.
  - IR: combine `add` and `sub` with constant.
  - ASM: combine `fadd`/`fsub` and `fmul` into `fmadd`, `fmsub`, etc.
  - ASM: remove back and forth moves.
  - [x] ASM: immediate for load and store instructions.
- [ ] Dead code elimination
  - IR & ASM: unused definitions
  - IR & ASM: jump to next instruction (basic block)
- [ ] Strength reduction
  - [x] IR: integer multiplication to shift.
  - [x] IR: integer division by 2 to shift.
  - [ ] IR: integer division by 2^k to shift.
- [ ] `memcpy` for local arrays.
- [ ] Loop invariant code motion
  - `performance/hoist`: sum is added 100 times and divided by 100.
