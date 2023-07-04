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

- [x] Flatten source code for submit
- [x] Command-line arguments
- [ ] Parser & Lexer
  - [x] Solve shift/reduce conflict of function and declaration.
    - Ref: [Midrule-Conflicts](https://www.gnu.org/software/bison/manual/html_node/Midrule-Conflicts.html)
  - [x] Add support for function from the runtime library.
  - [x] Fix symbol table for identifier.
  - [x] Expand macro for timing functions.
    - Not tested yet (AST display).
  - [x] Use `std::optional` to replace nullptr in nullable pointers (memory safety).
    - Change the nullptr in AST, symbol table and type to std::nullopt.
    - nullptr in the driver still remains.
  - [ ] Suppot string.
  - [ ] Replace YYABORT with better error handling.
- [x] AST display
  - [x] Compunit
  - [x] Statement
  - [x] Expression
  - [x] Prettify
- [x] IR Generation
  - [x] Compunit
  - [x] Stmt
    - [x] Decl
    - [x] FuncDef
    - [x] Return
    - [x] Assign
    - [x] Block
    - [x] If
    - [x] While
    - [x] Break
    - [x] Continue
    - [x] Expr
  - [x] Expr
    - [x] Binary
    - [x] Unary
    - [x] Constant
    - [x] Identifier
    - [x] Cast
    - [x] Call
    - [x] Initializer list
  - [x] Type
  - [x] Symbol entry -> operand
  - [x] Update def & use when removing instructions and basic blocks.
  - [x] Remove unused basic blocks.
- [ ] Code generation
- [ ] Code print
- [ ] AST simplification
  - [x] Compile-time value for initializer list
  - [ ] Constant folding
  - [ ] Constant propagation
- [ ] IR Optimization
  - [ ] Memset for most-zero initialization.
  - [ ] Dead code elimination.
  - [ ] Memcpy for most-comptime initialization.
- [ ] Code Optimization

## Fixme

- [x] Temporary symbol duplication in the symbol table.
  - Fixed. The actual problem is that if there is a while/if/else, the block will be added twice: once by the driver and another time by the while/if/else statement.
- [x] Fix `getelementptr` for array parameter that is treated as pointer (no first index).
- [x] Fix array-type parameter/argument passing.
- [x] Bool -> int conversion in unary expression.
- [x] Function symbol scope when generating ir, consider record the symbol in ast.
- [x] IR store type problem in functional testcases 95~99
- [x] Reshape initializer list in `hidden_functional/08_global_arr_init.sy`
- [ ] Passing array with a different dimension into a function as a pointer (e.g. using `getarray` to input a array with 2 dimensions)
- [ ] `86_long_code2.sy` SEGMENTATION FAULT.
