%option reentrant noyywrap nounput yylineno

%{

#include "common.h"
#include "frontend/ast.h"
#include "frontend/type.h"
#include "frontend/comptime.h"

using namespace syc;

#define YY_DECL frontend::Parser::symbol_type yylex( \
  yyscan_t yyscanner, frontend::location&loc, frontend::Driver& driver)

#define YY_USER_ACTION                      \
  loc.begin.line = loc.end.line = yylineno; \
  loc.begin.column = yycolumn;              \
  loc.end.column = yycolumn + yyleng - 1;   \
  yycolumn += yyleng;

#define yyterminate() return frontend::Parser::make_END(loc)

#include "frontend/generated/parser.h"

// using namespace yy;

using frontend::Parser;
using token = frontend::Parser::token;
using token_type = frontend::Parser::token_type;

%}

DEC ([1-9][0-9]*|0)
OCT (0[0-7]+)
HEX (0[xX][0-9A-Fa-f]+)

IDENTIFIER [A-Za-z_][A-Za-z0-9_]*

EOL (\r\n|\n|\r)

WS [\t ]

BLOCK_COMMENT_BEGIN \/\*
BLOCK_COMMENT_ELEMENT .
BLOCK_COMMENT_END \*\/

%x BLOCK_COMMENT

LINE_COMMENT \/\/[^\n]*

DEC_FP ((([0-9]*[.][0-9]*([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+))[fLlL]?)
HEX_FP (0[xX](([0-9A-Fa-f]*[.][0-9A-Fa-f]*([pP][+-]?[0-9]+)?)|([0-9A-Fa-f]+[pP][+-]?[0-9]+))[fLlL]?)

%%

%{
  loc.step();
%}

"+" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('+'), loc);
}

"-" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('-'), loc);
}

"*" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('*'), loc);
}

"/" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('/'), loc);
}

"%" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('%'), loc);
}

"=" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('='), loc);
}

"[" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('['), loc);
}

"]" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type(']'), loc);
}

"{" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('{'), loc);
}

"}" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('}'), loc);
}

"(" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('('), loc);
}

")" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type(')'), loc);
}

"!" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('!'), loc);
}

"," {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type(','), loc);
}

";" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type(';'), loc);
}

">" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('>'), loc);
}

"<" {
  driver.add_token(yytext);
  return Parser::symbol_type(token_type('<'), loc);
}

"<=" {
  driver.add_token("LE");
  return Parser::make_LE(loc);
}

">=" {
  driver.add_token("GE");
  return Parser::make_GE(loc);
}

"==" {
  driver.add_token("EQ");
  return Parser::make_EQ(loc);
}

"!=" {
  driver.add_token("NE");
  return Parser::make_NE(loc);
}

"||" {
  driver.add_token("LOR");
  return Parser::make_LOR(loc);
}

"&&" {
  driver.add_token("LAND");
  return Parser::make_LAND(loc);
}

"int" {
  driver.add_token("INT");
  return Parser::make_INT(loc);
}

"float" {
  driver.add_token("FLOAT");
  return Parser::make_FLOAT(loc);
}

"void" {
  driver.add_token("VOID");
  return Parser::make_VOID(loc);
}

"const" {
  driver.add_token("CONST");
  driver.is_curr_decl_const = true;
  return Parser::make_CONST(loc);
}

"if" {
  driver.add_token("IF");
  return Parser::make_IF(loc);
}

"else" {
  driver.add_token("ELSE");
  return Parser::make_ELSE(loc);
}

"return" {
  driver.add_token("RETURN");
  return Parser::make_RETURN(loc);
}

"break" {
  driver.add_token("BREAK");
  return Parser::make_BREAK(loc);
}

"continue" {
  driver.add_token("CONTINUE");
  return Parser::make_CONTINUE(loc);
}

"while" {
  driver.add_token("WHILE");
  return Parser::make_WHILE(loc);
}

{IDENTIFIER} {
  driver.add_token(std::string("IDENTIFIER ") + yytext);
  return Parser::make_IDENTIFIER(yytext, loc);
}

{DEC} {
  auto value = frontend::create_comptime_value(
    std::stoi(yytext, nullptr, 10), frontend::create_int_type());

  driver.add_token(value->to_string());

  return Parser::make_INTEGER(value, loc);
}

{OCT} {
  auto value = frontend::create_comptime_value(
    std::stoi(yytext, nullptr, 8), frontend::create_int_type());

  driver.add_token(value->to_string());
  
  return Parser::make_INTEGER(value, loc);
}

{HEX} {
  auto value = frontend::create_comptime_value(
    std::stoi(yytext, nullptr, 16), frontend::create_int_type());

  driver.add_token(value->to_string());

  return Parser::make_INTEGER(value, loc);
}

{DEC_FP} {
  auto value = frontend::create_comptime_value(
    std::stof(yytext, nullptr), frontend::create_float_type());

  driver.add_token(value->to_string());

  return Parser::make_FLOATING(value, loc);
}

{HEX_FP} {
  auto value = frontend::create_comptime_value(
    std::stof(yytext, nullptr), frontend::create_float_type());

  driver.add_token(value->to_string());

  return Parser::make_FLOATING(value, loc);
}

{EOL} { yycolumn = 1; }
{WS} {}
{LINE_COMMENT} {}
{BLOCK_COMMENT_BEGIN} {BEGIN BLOCK_COMMENT;}
<BLOCK_COMMENT>{BLOCK_COMMENT_ELEMENT} {}
<BLOCK_COMMENT>{BLOCK_COMMENT_END} {BEGIN INITIAL;}

%%