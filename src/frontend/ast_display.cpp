#include "frontend/ast.h"
#include "frontend/symtable.h"
#include "utils.h"

namespace syc {
namespace frontend {

namespace ast {

std::string Expr::to_string() const {
  return "";
}

std::string Stmt::to_string() const {
  std::stringstream buf;
  buf << "Stmt: ";

  std::visit(
    overloaded{
      [&buf](const stmt::Blank& kind) { buf << "Blank"; },
      [&buf](const stmt::If& kind) {
        buf << "If" << std::endl;
        buf << "  cond: " << std::endl;
        buf << indent_str(kind.cond->to_string(), "    ") << std::endl;
        buf << "  then_stmt: " << std::endl;
        buf << indent_str(kind.then_stmt->to_string(), "    ") << std::endl;
        if (kind.maybe_else_stmt != nullptr) {
          buf << "  else_stmt: " << std::endl;
          buf << indent_str(kind.maybe_else_stmt->to_string(), "    ")
              << std::endl;
        }
      },
      [&buf](const stmt::While& kind) {
        buf << "While" << std::endl;
        buf << "  cond:" << std::endl;
        buf << indent_str(kind.cond->to_string(), "    ") << std::endl;
        buf << "  body:" << std::endl;
        buf << indent_str(kind.body->to_string(), "    ") << std::endl;
      },
      [&buf](const stmt::Break& kind) { buf << "Break" << std::endl; },
      [&buf](const stmt::Continue& kind) { buf << "Continue" << std::endl; },
      [&buf](const stmt::Return& kind) {
        buf << "Return" << std::endl;
        if (kind.expr != nullptr) {
          buf << indent_str(kind.expr->to_string(), "  ") << std::endl;
        }
      },
      [&buf](const stmt::Assign& kind) {
        buf << "Assign" << std::endl;
        buf << indent_str(kind.lhs->to_string(), "  ") << std::endl;
        buf << indent_str(kind.rhs->to_string(), "  ") << std::endl;
      },
      [&buf](const stmt::Expr& kind) {
        buf << "Expr" << std::endl;
        buf << indent_str(kind.expr->to_string(), "  ") << std::endl;
      },
      [&buf](const stmt::Decl& kind) {
        buf << "Decl" << std::endl;
        switch (kind.scope) {
          case Scope::Global:
            buf << "global ";
            break;
          case Scope::Local:
            buf << "local ";
            break;
          case Scope::Param:
            buf << "param ";
            break;
          case Scope::Temp:
            buf << "temp ";
            break;
        }
        if (kind.is_const) {
          buf << "const";
        }
        buf << std::endl;
        for (const auto& [type, name, maybe_init] : kind.defs) {
          buf << "  Def" << std::endl;
          buf << indent_str(type->to_string(), "    ") << " " << name
              << std::endl;
          if (maybe_init != nullptr) {
            buf << indent_str(maybe_init->to_string(), "    ") << std::endl;
          }
        }
      },
      [&buf](const stmt::FuncDef& kind) {
        buf << "FuncDef" << std::endl;
        buf << indent_str(kind.symtable->to_string(), "  ") << std::endl;
        buf << indent_str(kind.symbol_entry->to_string(), "  ") << std::endl;
        // If the function is a declaration, body is nullptr.
        if (kind.body != nullptr) {
          buf << indent_str(kind.body->to_string(), "  ") << std::endl;
        }
      },
      [&buf](const stmt::Block& kind) {
        buf << "Block" << std::endl;
        buf << indent_str(kind.symtable->to_string(), "  ") << std::endl;
        for (auto stmt : kind.stmts) {
          buf << indent_str(stmt->to_string(), "  ") << std::endl;
        }
      },
      [&buf](const auto& kind) { buf << "123123"; },
    },
    this->kind
  );
  buf << std::endl;

  return buf.str();
}

std::string Compunit::to_string() const {
  std::stringstream buf;
  buf << "Compunit" << std::endl;

  buf << indent_str(this->symtable->to_string(), "  ") << std::endl
      << std::endl;

  for (auto stmt : this->stmts) {
    buf << indent_str(stmt->to_string(), "  ") << std::endl;
  }
  return buf.str();
}

}  // namespace ast

}  // namespace frontend
}  // namespace syc