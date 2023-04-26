#include "frontend/driver.h"

namespace syc {
namespace frontend {

Driver::Driver() {
  compunit = ast::Compunit();
  curr_block = nullptr;
  curr_function = nullptr;
  curr_symtable = compunit.symtable;
}

bool Driver::is_curr_global() const {
  return curr_function == nullptr;
}

void Driver::add_stmt(ast::StmtPtr stmt) {
  if (this->is_curr_global()) {
    this->compunit.add_stmt(stmt);
  } else {
    // TODO: Test if the statement can be actually added into the block.
    std::get<ast::stmt::Block>(this->curr_block->kind).add_stmt(stmt);
  }
}


}
}