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
  - t3 is used in codegen for temporary immediate loading and comparison.
  - t4 is used in argument passing (temporarily store the new sp).
  - t5, t6 and all temporary floating-point registers are not used yet.
- [ ] Fix `hidden_functional/30_many_dimensions` (segmentation fault)
- [ ] Fix `hidden_functional/38_light2d` (wrong answer)
