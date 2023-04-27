%require "3.2"

%language "C++"

%skeleton "lalr1.cc"
%defines

%define api.token.constructor
%define api.value.type variant

%locations

%define parse.trace
%define parse.error verbose

%define api.location.file "location.h"
%define api.location.include { "frontend/generated/location.h" }

%code requires {
  typedef void* yyscan_t;

  #include "common.h"
  #include "frontend/ast.h"
  #include "frontend/symtable.h"
  #include "frontend/type.h" 
  #include "frontend/driver.h"
  #include "frontend/comptime.h"

  using namespace syc;
}

%code {

  yy::parser::symbol_type yylex(
    yyscan_t yyscanner, 
    yy::location& loc, 
    frontend::Driver& driver
  );
}

%lex-param { yyscan_t yyscanner } { yy::location& loc } { frontend::Driver& driver }
%parse-param { yyscan_t yyscanner } { yy::location& loc } { frontend::Driver& driver }

%token END 0;

%token '+' '-' '*' '/' '%' '=' '[' ']' '(' ')' '{' '}' '!' ',' ';'
%token LE GE EQ NE LOR LAND

%token <std::string> IDENTIFIER 
%token <frontend::ComptimeValue> INTEGER FLOATING

%token IF ELSE WHILE RETURN BREAK CONTINUE 
%token CONST INT FLOAT VOID

%start Program

%type <std::vector<frontend::ast::StmtPtr>> Stmts

%type <frontend::ast::StmtPtr> Stmt IfStmt WhileStmt ReturnStmt BreakStmt 
%type <frontend::ast::StmtPtr> BlockStmt ContinueStmt

%type <frontend::ast::ExprPtr> Expr AddExpr LOrExpr PrimaryExpr RelExpr MulExpr
%type <frontend::ast::ExprPtr> LAndExpr EqExpr UnaryExpr IndexExpr

%type <std::tuple<AstTypePtr, std::string>> FuncParam
%type <std::vector<std::tuple<frontend::type::TypePtr, std::string>>> FuncParamList
%type <std::vector<frontend::ast::ExprPtr>> FuncArgList

%type <frontend::TypePtr> Type

%precedence THEN
%precedence ELSE

%%

Program 
  : Stmts
  ;

Stmts 
  : Stmt {
    driver.add_stmt($1);
  }
  | Stmts Stmt {
    driver.add_stmt($2);
  }
  ;

Stmt
  : IfStmt {
    $$ = $1;
  }
  | WhileStmt {
    $$ = $1;
  }
  | ReturnStmt {
    $$ = $1;
  }
  | ContinueStmt {
    $$ = $1;
  }
  | BreakStmt {
    $$ = $1;
  }
  ;

IfStmt 
  : IF '(' Expr ')' Stmt %prec THEN {
    $$ = frontend::ast::create_if_stmt($3, $5, nullptr);
  }
  | IF '(' Expr ')' Stmt ELSE Stmt {
    $$ = frontend::ast::create_if_stmt($3, $5, $7);
  }
  ;

WhileStmt 
  : WHILE '(' Expr ')' Stmt {
    $$ = frontend::ast::create_while_stmt($3, $5);
  }
  ;

ReturnStmt 
  : RETURN Expr {
    $$ = frontend::ast::create_return_stmt($2);
  }
  ;

ContinueStmt 
  : CONTINUE ';' {
    $$ = frontend::ast::create_continue_stmt();
  }
  ;

BreakStmt 
  : BREAK ';' {
    $$ = frontend::ast::create_break_stmt();
  }
  ;

BlockStmt
  : '{' {
    driver.add_block();
  } Stmts '}' {
    driver.quit_block();
  }
  | '{' '}' {
    // Just ignore.
  }
  ;

BlankStmt : ;


Expr 
  : AddExpr
  ;

PrimaryExpr
  : '(' Expr ')' {
    $$ = $2;
  }
  | IDENTIFIER {
    auto symbol_entry = driver.curr_symtable->lookup($1);
    if (symbol_entry == nullptr) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
    }
    $$ = frontend::ast::create_identifier_expr(symbol_entry);
  }
  | INTEGER {

  }
  | FLOATING {

  }
  ;

IndexExpr
  : Expr '[' Expr ']' {

  }
  ;

UnaryExpr
  : PrimaryExpr {
    $$ = $1;
  }
  | '+' UnaryExpr {
    $$ = frontend::ast::create_unary_expr(
      frontend::UnaryOp::Pos, $2, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | '-' UnaryExpr {
    $$ = frontend::ast::create_unary_expr(
      frontend::UnaryOp::Neg, $2, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | '!' UnaryExpr {
    $$ = frontend::ast::create_unary_expr(
      frontend::UnaryOp::LogicalNot, $2, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | IDENTIFIER '(' FuncArgList ')' {
    auto symbol_entry = driver.compunit.symtable->lookup($1);
    if (symbol_entry == nullptr) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
    }
    auto maybe_func_type = symbol_entry->type;
    if (
      !std::holds_alternative<frontend::type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << @1 << ":" << "Not a function: " + $1;
    }
    $$ = frontend::ast::create_call_expr(
      symbol_entry, 
      $3, 
      driver.get_next_temp_name(), 
      driver.curr_symtable
    );
  }
  | IDENTIFIER '(' ')' {
    auto symbol_entry = driver.compunit.symtable->lookup($1);
    if (symbol_entry == nullptr) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
    }
    auto maybe_func_type = symbol_entry->type;
    if (
      !std::holds_alternative<frontend::type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << @1 << ":" << "Not a function: " + $1;
    }
    $$ = frontend::ast::create_call_expr(
      symbol_entry, 
      {}, 
      driver.get_next_temp_name(), 
      driver.curr_symtable
    );
  }
  ;

MulExpr
  : UnaryExpr {
    $$ = $1;
  }
  | MulExpr '*' UnaryExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Mul, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | MulExpr '/' UnaryExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Div, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | MulExpr '%' UnaryExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Mod, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  ;

AddExpr
  : MulExpr {
    $$ = $1;
  }
  | AddExpr '+' MulExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Add, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | AddExpr '-' MulExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Sub, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  ;

RelExpr
  : AddExpr {
    $$ = $1;
  }
  | RelExpr '<' AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Lt, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | RelExpr '>' AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Gt, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | RelExpr LE AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Le, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | RelExpr GE AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Ge, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  ;

EqExpr
  : RelExpr {
    $$ = $1;
  }
  | EqExpr EQ RelExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Eq, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  | EqExpr NE RelExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Ne, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  ;

LAndExpr
  : EqExpr {
    $$ = $1;
  }
  | LAndExpr LAND EqExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::LogicalAnd, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  ;

LOrExpr
  : LAndExpr {
    $$ = $1;
  }
  | LOrExpr LOR LAndExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::LogicalOr, $1, $3, 
      driver.get_next_temp_name(), driver.curr_symtable);
  }
  ;

FuncArgList
  : Expr {
    $$ = std::vector<frontend::ast::ExprPtr>{ $1 };
  }
  | FuncArgList ',' Expr {
    $$ = $1;
    $$.push_back($3);
  }
  ;

FuncParamList 
  : FuncParam {
    $$ = std::vector<std::tuple<frontend::type::TypePtr, std::string>>{ $1 };
  }
  | FuncParamList ',' FuncParam {
    $$ = $1;
    $$.push_back($3);
  }
  ;

FuncParam
  : Type IDENTIFIER {
    $$ = std::make_tuple($1, $2)
  }
  | FuncParam '[' Expr ']' {
    if ($3->is_comptime()) {
      std::cerr << @3 << ":" << "Array size must be a constant expression";
    }
    auto size = $3->get_comptime_value();
    if (!size.has_value()) {
      std::cerr << @3 << ":" << "Array size must be computable at complie-time";
    }
    auto type = frontend::create_array_type(std::get<0>($1), size);
    $$ = std::make_tuple(type, std::get<1>($1));
  }
  | FuncParam '[' ']' {
    auto type = frontend::create_array_type(std::get<0>($1), std::nullopt);
    $$ = std::make_tuple(type, std::get<1>($1));
  }
  ;

Type
  : INT {
    $$ = frontend::create_int_type();
  }
  | FLOAT {
    $$ = frontend::create_float_type();
  }
  | VOID {
    $$ = frontend::create_void_type();
  }
  ;

%%

void yy::parser::error (const location_type& loc, const std::string& msg) {
  std::cerr << loc << ": " << msg << std::endl;
}