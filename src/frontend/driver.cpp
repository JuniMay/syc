#include "frontend/driver.h"

#include "frontend/generated/lexer.h"
#include "frontend/generated/parser.h"

namespace syc {
namespace frontend {

Driver::Driver(std::string filename) {
  compunit = ast::Compunit();
  curr_block = nullptr;
  curr_function = nullptr;
  curr_symtable = compunit.symtable;
  next_temp_id = 0;
  tokens = "";
  curr_decl_type = nullptr;

  yylex_init(&lexer);
  yyset_in(fopen(filename.c_str(), "r"), lexer);
  loc = new yy::location();
  parser = new yy::parser(lexer, *loc, *this);

  // now add functions of the runtime library.
  this->add_function_decl(create_int_type(), "getint", {});
  this->add_function_decl(create_int_type(), "getch", {});
  this->add_function_decl(create_float_type(), "getfloat", {});
  this->add_function_decl(
    create_int_type(), "getarray",
    {{create_array_type(create_int_type(), std::nullopt), ""}}
  );
  this->add_function_decl(
    create_int_type(), "getfarray",
    {{create_array_type(create_float_type(), std::nullopt), ""}}
  );
  this->add_function_decl(
    create_void_type(), "putint", {{create_int_type(), ""}}
  );
  this->add_function_decl(
    create_void_type(), "putch", {{create_int_type(), ""}}
  );
  this->add_function_decl(
    create_void_type(), "putfloat", {{create_float_type(), ""}}
  );
  this->add_function_decl(
    create_void_type(), "putarray",
    {{create_int_type(), ""},
     {create_array_type(create_int_type(), std::nullopt), ""}}
  );
  this->add_function_decl(
    create_void_type(), "putfarray",
    {{create_int_type(), ""},
     {create_array_type(create_float_type(), std::nullopt), ""}}
  );

  // `putf` function is not added yet.

  this->add_function_decl(
    create_void_type(), "_sysy_starttime", {{create_int_type(), "lineno"}}
  );
  this->add_function_decl(
    create_void_type(), "_sysy_stoptime", {{create_int_type(), "lineno"}}
  );
}

Driver::~Driver() {
  yylex_destroy(lexer);
  delete loc;
  delete parser;
}

bool Driver::is_curr_global() const {
  // Note that a function must be added to the statements of the comunit first
  // and then set to be the current function.
  return curr_function == nullptr;
}

void Driver::add_stmt(ast::StmtPtr stmt) {
  if (this->is_curr_global()) {
    this->compunit.add_stmt(stmt);
  } else {
    std::get<ast::stmt::Block>(this->curr_block->kind).add_stmt(stmt);
  }
}

void Driver::add_block() {
  if (this->is_curr_global()) {
    throw std::runtime_error("Cannot add a block in global scope.");
  }

  ast::StmtPtr new_block = ast::create_block_stmt(this->curr_symtable);

  if (this->curr_block == nullptr) {
    // Set the block as the function's body.
    std::get<ast::stmt::FuncDef>(this->curr_function->kind).set_body(new_block);
  } else {
    // Just add the block as a compound statement.
    this->add_stmt(new_block);
  }

  this->curr_block = new_block;
  this->curr_symtable =
    std::get<ast::stmt::Block>(this->curr_block->kind).symtable;

  this->block_stack.push(this->curr_block);
}

void Driver::quit_block() {
  if (this->is_curr_global()) {
    throw std::runtime_error("Cannot pop a block in global scope.");
  }

  // debug
  // std::cout << this->curr_symtable->to_string() << std::endl;

  this->block_stack.pop();

  if (this->block_stack.empty()) {
    this->curr_block = nullptr;
    this->curr_symtable =
      std::get<ast::stmt::FuncDef>(this->curr_function->kind).symtable;
  } else {
    this->curr_block = this->block_stack.top();
    this->curr_symtable =
      std::get<ast::stmt::Block>(this->curr_block->kind).symtable;
  }
}

void Driver::add_function(
  TypePtr ret_type,
  std::string name,
  std::vector<std::tuple<TypePtr, std::string>> params
) {
  if (!this->is_curr_global()) {
    throw std::runtime_error("Cannot add a function in a local context.");
  }

  auto new_function =
    ast::create_func_def_stmt(this->curr_symtable, ret_type, name, params);

  this->add_stmt(new_function);
  this->curr_function = new_function;
  this->curr_symtable =
    std::get<ast::stmt::FuncDef>(this->curr_function->kind).symtable;
}

void Driver::quit_function() {
  // debug
  // std::cout << this->curr_symtable->to_string() << std::endl;

  this->curr_block = nullptr;
  while (!this->block_stack.empty()) {
    this->block_stack.pop();
  }
  this->curr_function = nullptr;
  this->curr_symtable = this->compunit.symtable;
}

void Driver::add_function_decl(
  TypePtr ret_type,
  std::string name,
  std::vector<std::tuple<TypePtr, std::string>> params
) {
  this->add_function(ret_type, name, params);
  this->quit_function();
}

void Driver::add_token(const std::string& token) {
  this->tokens += token + "\n";
}

std::string Driver::get_next_temp_name() {
  std::stringstream buf;
  buf << "__SYC_TEMP_" << this->next_temp_id;
  next_temp_id++;
  return buf.str();
}

}  // namespace frontend
}  // namespace syc