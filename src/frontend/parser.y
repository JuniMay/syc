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
%define api.parser.class { Parser }
%define api.namespace { syc::frontend }

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
  syc::frontend::Parser::symbol_type yylex(
    yyscan_t yyscanner, 
    syc::frontend::location& loc, 
    syc::frontend::Driver& driver
  );
}

%lex-param { yyscan_t yyscanner } { location& loc } { Driver& driver }
%parse-param { yyscan_t yyscanner } { location& loc } { Driver& driver }

%token END 0;

%token '+' '-' '*' '/' '%' '=' '[' ']' '(' ')' '{' '}' '!' ',' ';'
%token LE GE EQ NE LOR LAND

%token <std::string> IDENTIFIER 
%token <ComptimeValuePtr> INTEGER FLOATING

%token IF ELSE WHILE RETURN BREAK CONTINUE 
%token CONST INT FLOAT VOID

%start Program

%type <ast::StmtPtr> Stmt IfStmt WhileStmt ReturnStmt BreakStmt ExprStmt
%type <ast::StmtPtr> ContinueStmt BlankStmt DeclStmt AssignStmt BlockStmt

%type <std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> Def
%type <std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>>> DefList

%type <ast::ExprPtr> Expr AddExpr LOrExpr PrimaryExpr RelExpr MulExpr
%type <ast::ExprPtr> LAndExpr EqExpr UnaryExpr LVal InitVal Cond

%type <std::vector<ast::ExprPtr>> InitValList

%type <std::tuple<TypePtr, std::string>> FuncParam
%type <std::vector<std::tuple<TypePtr, std::string>>> FuncParamList
%type <std::vector<ast::ExprPtr>> FuncArgList

%type <TypePtr> Type 
%type <std::vector<ast::ExprPtr>> ArrayIndices

%precedence THEN
%precedence ELSE


%%

Program 
  : Stmts
  ;

Stmts 
  : Stmt {
    if ($1 != nullptr) {
      driver.add_stmt($1);
    }
  }
  | Stmts Stmt {
    if ($2 != nullptr) {
      driver.add_stmt($2);
    }
  }
  ;

Stmt
  : AssignStmt {
    $$ = $1;
  }
  | IfStmt {
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
  | DeclStmt {
    $$ = $1;
  }
  | ExprStmt {
    $$ = $1;
  }
  | BlankStmt {
    $$ = $1;
  }
  | BlockStmt {
    $$ = $1;
  }
  | FuncDef {
    $$ = nullptr;
  }
  ;

FuncDef 
  : Type IDENTIFIER '(' ')' {
    driver.add_function($1, $2, {});
  } BlockStmt {
    driver.quit_function();
  }
  | Type IDENTIFIER '(' FuncParamList ')' {
    driver.add_function($1, $2, $4);
  } BlockStmt {
    driver.quit_function();
  }
  ;

ExprStmt
  : Expr ';' {
    $$ = ast::create_expr_stmt($1);
  }
  ;

DeclStmt
  : Type DefList ';' {
    $$ = ast::create_decl_stmt(driver.curr_decl_scope, false, $2);
  }
  | CONST Type DefList ';' {
    $$ = ast::create_decl_stmt(driver.curr_decl_scope, true, $3);
    driver.is_curr_decl_const = false;
  }
  ;

DefList
  : DefList ',' Def {
    $$ = $1;
    $$.push_back($3);

    auto symbol_entry = ast::create_symbol_entry_from_decl_def(
      driver.curr_decl_scope,
      driver.is_curr_decl_const,
      $3
    );
    driver.curr_symtable->add_symbol_entry(symbol_entry);
  }
  | Def {
    $$.push_back($1);

    auto symbol_entry = ast::create_symbol_entry_from_decl_def(
      driver.curr_decl_scope,
      driver.is_curr_decl_const,
      $1
    );
    driver.curr_symtable->add_symbol_entry(symbol_entry);
  }
  ;

Def 
  : IDENTIFIER {
    $$ = std::make_tuple(driver.curr_decl_type, $1, std::nullopt);
  } 
  | IDENTIFIER ArrayIndices {
    TypePtr decl_type = driver.curr_decl_type;

    auto curr_index_expr = $2.rbegin();
    while (curr_index_expr != $2.rend()) {
      auto maybe_decl_type = create_array_type_from_expr(
        decl_type, *curr_index_expr);
      if (!maybe_decl_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      decl_type = maybe_decl_type.value();
      ++curr_index_expr;
    }

    $$ = std::make_tuple(decl_type, $1, std::nullopt);
  }
  | IDENTIFIER '=' InitVal {
    auto init_val = $3;
    if (init_val->is_initializer_list()) {
      std::get<ast::expr::InitializerList>(init_val->kind)
        .set_type(driver.curr_decl_type);
    }
    $$ = std::make_tuple(
      driver.curr_decl_type, 
      $1, 
      std::make_optional(init_val)
    );
  }
  | IDENTIFIER ArrayIndices '=' InitVal {
    TypePtr decl_type = driver.curr_decl_type;

    auto curr_index_expr = $2.rbegin();
    while (curr_index_expr != $2.rend()) {
      auto maybe_decl_type = create_array_type_from_expr(
        decl_type, *curr_index_expr);
      if (!maybe_decl_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      decl_type = maybe_decl_type.value();
      ++curr_index_expr;
    }

    auto init_val = $4;
    if (init_val->is_initializer_list()) {
      std::get<ast::expr::InitializerList>(init_val->kind).set_type(decl_type);
    }
    $$ = std::make_tuple(decl_type, $1, std::make_optional(init_val));
  }
  ;

ArrayIndices
  : '[' Expr ']' {
    $$ = { $2 };
  }
  | ArrayIndices '[' Expr ']' {
    $$ = $1;
    $$.push_back($3);
  }
  ;

InitVal 
  : Expr {
    $$ = $1;
  }
  | '{' '}' {
    $$ = ast::create_initializer_list_expr({});
  }
  | '{' InitValList '}' {
    $$ = ast::create_initializer_list_expr($2);
  }
  ;

InitValList 
  : InitVal {
    $$.push_back($1);
  }
  | InitValList ',' InitVal {
    $$ = $1;
    $$.push_back($3);
  } 
  ;

IfStmt 
  : IF '(' Cond ')' Stmt %prec THEN {
    $$ = ast::create_if_stmt($3, $5, driver, std::nullopt);
  }
  | IF '(' Cond ')' Stmt ELSE Stmt {
    $$ = ast::create_if_stmt($3, $5, driver, std::make_optional($7));
  }
  ;

WhileStmt 
  : WHILE '(' Cond ')' Stmt {
    $$ = ast::create_while_stmt($3, $5, driver);
  }
  ;

ReturnStmt 
  : RETURN Expr ';' {
    $$ = ast::create_return_stmt(std::make_optional($2));
  }
  | RETURN ';' {
    $$ = ast::create_return_stmt(std::nullopt);
  }
  ;

ContinueStmt 
  : CONTINUE ';' {
    $$ = ast::create_continue_stmt();
  }
  ;

BreakStmt 
  : BREAK ';' {
    $$ = ast::create_break_stmt();
  }
  ;

BlockStmt
  : '{' {
    driver.add_block();
  } Stmts '}' {
    $$ = driver.curr_block;
    driver.quit_block();
  }
  | '{' '}' {
    // Just ignore.
    $$ = ast::create_blank_stmt();
  }
  ;

BlankStmt 
  : ';' {
    $$ = ast::create_blank_stmt();
  }
  ;


Expr 
  : AddExpr {
    $$ = $1;
  }
  ;

Cond
  : LOrExpr {
    $$ = $1;
  }
  ;

PrimaryExpr
  : '(' Expr ')' {
    $$ = $2;
  }
  | LVal {
    $$ = $1;
  }
  | INTEGER {
    $$ = ast::create_constant_expr($1);
  }
  | FLOATING {
    $$ = ast::create_constant_expr($1);
  }
  ;

LVal
  : IDENTIFIER {
    auto maybe_symbol_entry = driver.curr_symtable->lookup($1);
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
      YYABORT;
    }
    $$ = ast::create_identifier_expr(maybe_symbol_entry.value());
  }
  | LVal '[' Expr ']' {
    $$ = ast::create_binary_expr(
      BinaryOp::Index, $1, $3, driver
    );
  }
  ;

AssignStmt
  : LVal '=' Expr ';' {
    $$ = ast::create_assign_stmt($1, $3);
  }
  ;

UnaryExpr
  : PrimaryExpr {
    $$ = $1;
  }
  | '+' UnaryExpr {
    $$ = ast::create_unary_expr(UnaryOp::Pos, $2, driver);
  }
  | '-' UnaryExpr {
    $$ = ast::create_unary_expr(UnaryOp::Neg, $2, driver);
  }
  | '!' UnaryExpr {
    $$ = ast::create_unary_expr(UnaryOp::LogicalNot, $2, driver);
  }
  | IDENTIFIER '(' FuncArgList ')' {
    auto maybe_symbol_entry = driver.compunit.symtable->lookup($1);
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
      YYABORT;
    }
    auto maybe_func_type = maybe_symbol_entry.value()->type;
    if (
      !std::holds_alternative<type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << @1 << ":" << "Not a function: " + $1;
      YYABORT;
    }
    $$ = ast::create_call_expr(
      maybe_symbol_entry.value(), 
      $3, 
      driver
    );
  }
  | IDENTIFIER '(' ')' {
    if ($1 == "starttime" || $1 == "stoptime") {
      auto maybe_symbol_entry = driver.compunit.symtable->lookup("_sysy_" + $1);
      int lineno = @1.end.line;
      auto lineno_expr = ast::create_constant_expr(
        create_comptime_value(lineno, create_int_type()));
      $$ = ast::create_call_expr(
        maybe_symbol_entry.value(), 
        {lineno_expr}, 
        driver
      );
      YYACCEPT;
    }
    auto maybe_symbol_entry = driver.compunit.symtable->lookup($1);
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
      YYABORT;
    }
    auto maybe_func_type = maybe_symbol_entry.value()->type;
    if (
      !std::holds_alternative<type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << @1 << ":" << "Not a function: " + $1;
      YYABORT;
    }
    $$ = ast::create_call_expr(
      maybe_symbol_entry.value(), 
      {}, 
      driver
    );
  }
  ;

MulExpr
  : UnaryExpr {
    $$ = $1;
  }
  | MulExpr '*' UnaryExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Mul, $1, $3, driver);
  }
  | MulExpr '/' UnaryExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Div, $1, $3, driver);
  }
  | MulExpr '%' UnaryExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Mod, $1, $3, driver);
  }
  ;

AddExpr
  : MulExpr {
    $$ = $1;
  }
  | AddExpr '+' MulExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Add, $1, $3, driver);
  }
  | AddExpr '-' MulExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Sub, $1, $3, driver);
  }
  ;

RelExpr
  : AddExpr {
    $$ = $1;
  }
  | RelExpr '<' AddExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Lt, $1, $3, driver);
  }
  | RelExpr '>' AddExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Gt, $1, $3, driver);
  }
  | RelExpr LE AddExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Le, $1, $3, driver);
  }
  | RelExpr GE AddExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Ge, $1, $3, driver);
  }
  ;

EqExpr
  : RelExpr {
    $$ = $1;
  }
  | EqExpr EQ RelExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Eq, $1, $3, driver);
  }
  | EqExpr NE RelExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::Ne, $1, $3, driver);
  }
  ;

LAndExpr
  : EqExpr {
    $$ = $1;
  }
  | LAndExpr LAND EqExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::LogicalAnd, $1, $3, driver);
  }
  ;

LOrExpr
  : LAndExpr {
    $$ = $1;
  }
  | LOrExpr LOR LAndExpr {
    $$ = ast::create_binary_expr(
      BinaryOp::LogicalOr, $1, $3, driver);
  }
  ;

FuncArgList
  : Expr {
    $$.push_back($1);
  }
  | FuncArgList ',' Expr {
    $$ = $1;
    $$.push_back($3);
  }
  ;

FuncParamList 
  : FuncParam {
    $$.push_back($1);
  }
  | FuncParamList ',' FuncParam {
    $$ = $1;
    $$.push_back($3);
  }
  ;

FuncParam
  : Type IDENTIFIER {
    $$ = std::make_tuple($1, $2);
  }
  | Type IDENTIFIER ArrayIndices {
    TypePtr type = $1;

    auto curr_index_expr = $3.rbegin();
    while (curr_index_expr != $3.rend()) {
      auto maybe_type = create_array_type_from_expr(
        type, *curr_index_expr);
      if (!maybe_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      type = maybe_type.value();
      ++curr_index_expr;
    }

    $$ = std::make_tuple(type, $2);
  }
  | Type IDENTIFIER '[' ']' {
    $$ = std::make_tuple(create_array_type($1, std::nullopt), $2);
  }
  | Type IDENTIFIER '[' ']' ArrayIndices {
    TypePtr type = $1;

    auto curr_index_expr = $5.rbegin();
    while (curr_index_expr != $5.rend()) {
      auto maybe_type = create_array_type_from_expr(
        type, *curr_index_expr);
      if (!maybe_type.has_value()) {
        std::cerr << @2 << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      type = maybe_type.value();
      ++curr_index_expr;
    }

    type = create_array_type(type, std::nullopt);

    $$ = std::make_tuple(type, $2);
  }
  ;

Type
  : INT {
    $$ = create_int_type();
    driver.curr_decl_type = $$;
    driver.curr_decl_scope = driver.is_curr_global() ? Scope::Global 
                                                     : Scope::Local;

  }
  | FLOAT {
    $$ = create_float_type();
    driver.curr_decl_type = $$;
    driver.curr_decl_scope = driver.is_curr_global() ? Scope::Global 
                                                     : Scope::Local;
  }
  | VOID {
    $$ = create_void_type();
    driver.curr_decl_type = $$;
    driver.curr_decl_scope = driver.is_curr_global() ? Scope::Global 
                                                     : Scope::Local;
  }
  ;

%%

namespace syc {
namespace frontend {

void Parser::error (const location_type& loc, const std::string& msg) {
  std::cerr << loc << ": " << msg << std::endl;
}

} // namespace frontend
} // namespace syc
