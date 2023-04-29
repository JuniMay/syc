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
  - [ ] Suppot string.
  - [ ] Replace YYABORT by better error handling.
- [ ] AST display
- [ ] IR Generation
- [ ] Code generation
- [ ] Code print
- [ ] AST simplification
- [ ] IR Optimization
- [ ] Code Optimization
