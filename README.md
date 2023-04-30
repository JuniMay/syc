# SysY Compiler

## Getting Started

1. Generate the parser and lexer using `yacc_gen.sh` under scripts directory.
2. Build using cmake.

## Todo List

- [ ] Flatten source code for submit
- [ ] Commandline arguments
- [ ] Parser & Lexer
  - [x] Solve shift/reduce conflict of function and declaration.
    - Ref: [Midrule-Conflicts](https://www.gnu.org/software/bison/manual/html_node/Midrule-Conflicts.html)
  - [x] Add support for function from the runtime library.
  - [x] Fix symbol table for identifier.
  - [x] Expand macro for timing functions.
    - Not tested yet (AST display).
  - [ ] Use `std::optional` to replace nullptr in nullable pointers (memory safety).
  - [ ] Suppot string.
  - [ ] Replace YYABORT with better error handling.
- [ ] AST display
  - [x] Compunit
  - [x] Statement
  - [x] Expression
  - [ ] Prettify
- [ ] IR Generation
- [ ] Code generation
- [ ] Code print
- [ ] AST simplification
- [ ] IR Optimization
- [ ] Code Optimization

## Fixme

- [x] Temporary symbol duplication in the symbol table.
  - Fixed. The actual problem is that if there is a while/if/else, the block will be added twice: once by the driver and another time by the while/if/else statement.
