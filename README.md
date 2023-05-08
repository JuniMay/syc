# SysY Compiler

## Getting Started

### Development

1. Generate the parser and lexer using `yacc_gen.sh` under scripts directory.
2. Build using cmake.

### Submission

The machanism of the contest is compiling everything altogether. So we need to flatten the source code.

```shell
python3 scripts/flatten.py --input_dir ./src --output_dir ./flattened
python3 scripts/test_flattened.py
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
- [ ] IR Generation
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
  - [ ] Expr
    - [x] Binary
    - [x] Unary
    - [x] Constant
    - [x] Identifier
    - [x] Cast
    - [x] Call
    - [ ] Initializer list
      - Memcpy or multiple assignment
  - [x] Type
  - [x] Symbol entry -> operand
  - [ ] Update def & use when removing instructions and basic blocks.
  - [ ] Remove unused basic blocks.
- [ ] Code generation
- [ ] Code print
- [ ] AST simplification
  - [x] Compile-time value for initializer list
  - [ ] Constant folding
  - [ ] Constant propagation
- [ ] IR Optimization
- [ ] Code Optimization

## Fixme

- [x] Temporary symbol duplication in the symbol table.
  - Fixed. The actual problem is that if there is a while/if/else, the block will be added twice: once by the driver and another time by the while/if/else statement.
- [x] Fix `getelementptr` for array parameter that is treated as pointer (no first index).
- [x] Fix array-type parameter/argument passing.
