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

  using AstStmtPtr = frontend::ast::StmtPtr;
  using AstExprPtr = frontend::ast::ExprPtr;
  using AstTypePtr = frontend::TypePtr;
  using AstBinaryOp = frontend::BinaryOp;
  using AstUnaryOp = frontend::UnaryOp;
  using AstComptimeValue = frontend::ComptimeValue;

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
%token <AstComptimeValue> INTEGER FLOATING

%token IF ELSE WHILE RETURN BREAK CONTINUE 
%token CONST INT FLOAT VOID

%start Program

%type <AstStmtPtr> Stmt IfStmt WhileStmt ReturnStmt BreakStmt ExprStmt
%type <AstStmtPtr> ContinueStmt BlankStmt DeclStmt AssignStmt BlockStmt

%type <std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> Def
%type <std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>>> DefList

%type <AstExprPtr> Expr AddExpr LOrExpr PrimaryExpr RelExpr MulExpr
%type <AstExprPtr> LAndExpr EqExpr UnaryExpr LVal InitVal Cond

%type <std::vector<AstExprPtr>> InitValList

%type <std::tuple<AstTypePtr, std::string>> FuncParam
%type <std::vector<std::tuple<AstTypePtr, std::string>>> FuncParamList
%type <std::vector<AstExprPtr>> FuncArgList

%type <AstTypePtr> Type ArrayIndices

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
    $$ = frontend::ast::create_expr_stmt($1);
  }
  ;

DeclStmt
  : Type DefList ';' {
    auto scope = driver.is_curr_global() ? frontend::Scope::Global 
                                         : frontend::Scope::Local;
    $$ = frontend::ast::create_decl_stmt(scope, false, $2);
  }
  | CONST Type DefList ';' {
    auto scope = driver.is_curr_global() ? frontend::Scope::Global 
                                         : frontend::Scope::Local;
    $$ = frontend::ast::create_decl_stmt(scope, true, $3);
  }
  ;

DefList
  : DefList ',' Def {
    $$ = $1;
    $$.push_back($3);
  }
  | Def {
    $$.push_back($1);
  }
  ;

Def 
  : IDENTIFIER {
    $$ = std::make_tuple(driver.curr_decl_type, $1, std::nullopt);
  } 
  | IDENTIFIER ArrayIndices {
    $$ = std::make_tuple($2, $1, std::nullopt);
  }
  | IDENTIFIER '=' InitVal {
    auto init_val = $3;
    if (init_val->is_initializer_list()) {
      std::get<frontend::ast::expr::InitializerList>(init_val->kind)
        .set_type(driver.curr_decl_type);
    }
    $$ = std::make_tuple(
      driver.curr_decl_type, 
      $1, 
      std::make_optional(init_val)
    );
  }
  | IDENTIFIER ArrayIndices '=' InitVal {
    auto init_val = $4;
    if (init_val->is_initializer_list()) {
      std::get<frontend::ast::expr::InitializerList>(init_val->kind)
        .set_type($2);
    }
    $$ = std::make_tuple($2, $1, std::make_optional(init_val));
  }
  ;

ArrayIndices
  : '[' Expr ']' {
    // Get the corresponding array type from the `curr_decl_type`.
    auto maybe_type = frontend::create_array_type_from_expr(
      driver.curr_decl_type, std::make_optional($2));
    if (!maybe_type.has_value()) {
      std::cerr << @2 << ":" 
                << "Array size must be const expression." << std::endl;
      YYABORT;
    }
    $$ = maybe_type.value();
  }
  | ArrayIndices '[' Expr ']' {
    auto maybe_type = frontend::create_array_type_from_expr(
      $1, std::make_optional($3));
    if (!maybe_type.has_value()) {
      std::cerr << @3 << ":" 
                << "Array size must be const expression." << std::endl;
      YYABORT;
    }
    $$ = maybe_type.value();
  }
  ;

InitVal 
  : Expr {
    $$ = $1;
  }
  | '{' '}' {
    $$ = frontend::ast::create_initializer_list_expr({});
  }
  | '{' InitValList '}' {
    $$ = frontend::ast::create_initializer_list_expr($2);
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
    $$ = frontend::ast::create_if_stmt($3, $5, std::nullopt);
  }
  | IF '(' Cond ')' Stmt ELSE Stmt {
    $$ = frontend::ast::create_if_stmt($3, $5, std::make_optional($7));
  }
  ;

WhileStmt 
  : WHILE '(' Cond ')' Stmt {
    $$ = frontend::ast::create_while_stmt($3, $5);
  }
  ;

ReturnStmt 
  : RETURN Expr ';' {
    $$ = frontend::ast::create_return_stmt(std::make_optional($2));
  }
  | RETURN ';' {
    $$ = frontend::ast::create_return_stmt(std::nullopt);
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
    $$ = driver.curr_block;
    driver.quit_block();
  }
  | '{' '}' {
    // Just ignore.
    $$ = frontend::ast::create_blank_stmt();
  }
  ;

BlankStmt 
  : ';' {
    $$ = frontend::ast::create_blank_stmt();
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
    $$ = frontend::ast::create_constant_expr($1);
  }
  | FLOATING {
    $$ = frontend::ast::create_constant_expr($1);
  }
  ;

LVal
  : IDENTIFIER {
    auto maybe_symbol_entry = driver.curr_symtable->lookup($1);
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
      YYABORT;
    }
    $$ = frontend::ast::create_identifier_expr(maybe_symbol_entry.value());
  }
  | LVal '[' Expr ']' {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Index, $1, $3, driver
    );
  }
  ;

AssignStmt
  : LVal '=' Expr ';' {
    $$ = frontend::ast::create_assign_stmt($1, $3);
  }
  ;

UnaryExpr
  : PrimaryExpr {
    $$ = $1;
  }
  | '+' UnaryExpr {
    $$ = frontend::ast::create_unary_expr(
      frontend::UnaryOp::Pos, $2, driver);
  }
  | '-' UnaryExpr {
    $$ = frontend::ast::create_unary_expr(
      frontend::UnaryOp::Neg, $2, driver);
  }
  | '!' UnaryExpr {
    $$ = frontend::ast::create_unary_expr(
      frontend::UnaryOp::LogicalNot, $2, driver);
  }
  | IDENTIFIER '(' FuncArgList ')' {
    auto maybe_symbol_entry = driver.compunit.symtable->lookup($1);
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << @1 << ":" << "Undefined identifier: " + $1;
      YYABORT;
    }
    auto maybe_func_type = maybe_symbol_entry.value()->type;
    if (
      !std::holds_alternative<frontend::type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << @1 << ":" << "Not a function: " + $1;
      YYABORT;
    }
    $$ = frontend::ast::create_call_expr(
      maybe_symbol_entry.value(), 
      $3, 
      driver
    );
  }
  | IDENTIFIER '(' ')' {
    if ($1 == "starttime" || $1 == "stoptime") {
      auto maybe_symbol_entry = driver.compunit.symtable->lookup("_sysy_" + $1);
      int lineno = @1.end.line;
      auto lineno_expr = frontend::ast::create_constant_expr(
        frontend::create_comptime_value(lineno, frontend::create_int_type()));
      $$ = frontend::ast::create_call_expr(
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
      !std::holds_alternative<frontend::type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << @1 << ":" << "Not a function: " + $1;
      YYABORT;
    }
    $$ = frontend::ast::create_call_expr(
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
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Mul, $1, $3, driver);
  }
  | MulExpr '/' UnaryExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Div, $1, $3, driver);
  }
  | MulExpr '%' UnaryExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Mod, $1, $3, driver);
  }
  ;

AddExpr
  : MulExpr {
    $$ = $1;
  }
  | AddExpr '+' MulExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Add, $1, $3, driver);
  }
  | AddExpr '-' MulExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Sub, $1, $3, driver);
  }
  ;

RelExpr
  : AddExpr {
    $$ = $1;
  }
  | RelExpr '<' AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Lt, $1, $3, driver);
  }
  | RelExpr '>' AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Gt, $1, $3, driver);
  }
  | RelExpr LE AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Le, $1, $3, driver);
  }
  | RelExpr GE AddExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Ge, $1, $3, driver);
  }
  ;

EqExpr
  : RelExpr {
    $$ = $1;
  }
  | EqExpr EQ RelExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Eq, $1, $3, driver);
  }
  | EqExpr NE RelExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Ne, $1, $3, driver);
  }
  ;

LAndExpr
  : EqExpr {
    $$ = $1;
  }
  | LAndExpr LAND EqExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::LogicalAnd, $1, $3, driver);
  }
  ;

LOrExpr
  : LAndExpr {
    $$ = $1;
  }
  | LOrExpr LOR LAndExpr {
    $$ = frontend::ast::create_binary_expr(
      frontend::BinaryOp::LogicalOr, $1, $3, driver);
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
  | FuncParam '[' Expr ']' {
    auto maybe_type = frontend::create_array_type_from_expr(
      std::get<0>($1), std::make_optional($3));

    if (!maybe_type.has_value()) {
      std::cerr << @3 << ":" << "Array size must be const expression." << std::endl;
      YYABORT;
    }

    auto type = maybe_type.value();

    $$ = std::make_tuple(type, std::get<1>($1));
  }
  | FuncParam '[' ']' {
    auto type = frontend::create_array_type_from_expr(
      std::get<0>($1), std::nullopt).value();
    $$ = std::make_tuple(type, std::get<1>($1));
  }
  ;

Type
  : INT {
    $$ = frontend::create_int_type();
    driver.curr_decl_type = $$;
  }
  | FLOAT {
    $$ = frontend::create_float_type();
    driver.curr_decl_type = $$;
  }
  | VOID {
    $$ = frontend::create_void_type();
    driver.curr_decl_type = $$;
  }
  ;

%%

void yy::parser::error (const location_type& loc, const std::string& msg) {
  std::cerr << loc << ": " << msg << std::endl;
}