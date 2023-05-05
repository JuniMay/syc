#include "frontend/ast.h"
#include "frontend/symtable.h"
#include "utils.h"

namespace syc {
namespace frontend {

namespace ast {

std::string Expr::to_string() const {
  std::stringstream buf;
  buf << "Expr ";

  std::visit(

    overloaded{
      [&buf, this](const expr::Identifier& kind) {
        buf << "Identifier " << kind.name << std::endl;
        buf << indent_str(kind.symbol->to_string(), "\t") << std::endl;
      },
      [&buf, this](const expr::Binary& kind) {
        buf << "Binary ";
        switch (kind.op) {
          case BinaryOp::Add:
            buf << "Add";
            break;
          case BinaryOp::Sub:
            buf << "Sub";
            break;
          case BinaryOp::Mul:
            buf << "Mul";
            break;
          case BinaryOp::Div:
            buf << "Div";
            break;
          case BinaryOp::Mod:
            buf << "Mod";
            break;
          case BinaryOp::Lt:
            buf << "Lt";
            break;
          case BinaryOp::Gt:
            buf << "Gt";
            break;
          case BinaryOp::Le:
            buf << "Le";
            break;
          case BinaryOp::Ge:
            buf << "Ge";
            break;
          case BinaryOp::Eq:
            buf << "Eq";
            break;
          case BinaryOp::Ne:
            buf << "Ne";
            break;
          case BinaryOp::LogicalAnd:
            buf << "LogicalAnd";
            break;
          case BinaryOp::LogicalOr:
            buf << "LogicalOr";
            break;
          case BinaryOp::Index:
            buf << "Index";
            break;
        }
        buf << " " << kind.symbol->name << std::endl;
        buf << indent_str(kind.lhs->to_string(), "\t") << std::endl;
        buf << indent_str(kind.rhs->to_string(), "\t") << std::endl;
      },
      [&buf, this](const expr::Unary& kind) {
        buf << "Unary";
        switch (kind.op) {
          case UnaryOp::Pos:
            buf << "Pos";
            break;
          case UnaryOp::Neg:
            buf << "Neg";
            break;
          case UnaryOp::LogicalNot:
            buf << "LogicalNot";
            break;
        }
        buf << " " << kind.symbol->name << std::endl;
        buf << indent_str(kind.expr->to_string(), "\t") << std::endl;
      },
      [&buf, this](const expr::Call& kind) {
        buf << "Call " << kind.symbol->name << std::endl;
        buf << "  FUNCTION: " << kind.name << std::endl;
        for (auto expr : kind.args) {
          buf << indent_str(expr->to_string(), "\t") << std::endl;
        }
      },
      [&buf, this](const expr::Cast& kind) {
        buf << "Cast " << kind.symbol->name << std::endl;
        buf << indent_str(kind.expr->to_string(), "\t") << std::endl;
        buf << indent_str(kind.type->to_string(), "\t") << std::endl;
      },
      [&buf, this](const expr::Constant& kind) {
        buf << "Constant" << std::endl;
        buf << indent_str(kind.value->to_string(), "\t") << std::endl;
      },
      [&buf, this](const expr::InitializerList& kind) {
        buf << "InitializerList" << std::endl;
        for (auto expr : kind.init_list) {
          buf << indent_str(expr->to_string(), "\t") << std::endl;
        }
      },
      [&buf](const auto& kind) { buf << "UNKNWON_STMT"; },
    },
    this->kind
  );

  return buf.str();
}

std::string Stmt::to_string() const {
  std::stringstream buf;
  buf << "Stmt ";

  std::visit(
    overloaded{
      [&buf](const stmt::Blank& kind) { buf << "Blank" << std::endl; },
      [&buf](const stmt::If& kind) {
        buf << "If" << std::endl;
        buf << "  COND: " << std::endl;
        buf << indent_str(kind.cond->to_string(), "\t") << std::endl;
        buf << "  THEN: " << std::endl;
        buf << indent_str(kind.then_stmt->to_string(), "\t") << std::endl;
        if (kind.maybe_else_stmt.has_value()) {
          buf << "  ELSE: " << std::endl;
          buf << indent_str(kind.maybe_else_stmt.value()->to_string(), "\t")
              << std::endl;
        }
      },
      [&buf](const stmt::While& kind) {
        buf << "While" << std::endl;
        buf << "  COND:" << std::endl;
        buf << indent_str(kind.cond->to_string(), "\t") << std::endl;
        buf << "  BODY:" << std::endl;
        buf << indent_str(kind.body->to_string(), "\t") << std::endl;
      },
      [&buf](const stmt::Break& kind) { buf << "Break" << std::endl; },
      [&buf](const stmt::Continue& kind) { buf << "Continue" << std::endl; },
      [&buf](const stmt::Return& kind) {
        buf << "Return" << std::endl;
        if (kind.maybe_expr.has_value()) {
          buf << indent_str(kind.maybe_expr.value()->to_string(), "\t")
              << std::endl;
        } else {
          buf << "VOID" << std::endl;
        }
      },
      [&buf](const stmt::Assign& kind) {
        buf << "Assign" << std::endl;
        buf << indent_str(kind.lhs->to_string(), "\t") << std::endl;
        buf << indent_str(kind.rhs->to_string(), "\t") << std::endl;
      },
      [&buf](const stmt::Expr& kind) {
        buf << "Expr" << std::endl;
        buf << indent_str(kind.expr->to_string(), "\t") << std::endl;
      },
      [&buf](const stmt::Decl& kind) {
        buf << "Decl" << std::endl;
        switch (kind.scope) {
          case Scope::Global:
            buf << "  GLOBAL ";
            break;
          case Scope::Local:
            buf << "  LOCAL ";
            break;
          case Scope::Param:
            buf << "  PARAM ";
            break;
          case Scope::Temp:
            buf << "  TEMP ";
            break;
        }
        if (kind.is_const) {
          buf << "CONST ";
        }
        buf << std::endl;
        for (const auto& [type, name, maybe_init] : kind.defs) {
          buf << indent_str(type->to_string(), "\t") << " " << name
              << std::endl;
          if (maybe_init.has_value()) {
            buf << indent_str(maybe_init.value()->to_string(), "\t\t")
                << std::endl;
          }
        }
      },
      [&buf](const stmt::FuncDef& kind) {
        buf << "FuncDef" << std::endl;
        buf << indent_str(kind.symtable->to_string(), "\t") << std::endl
            << std::endl;
        buf << indent_str(kind.symbol_entry->to_string(), "\t") << std::endl;
        // If the function is a declaration, body is nullopt.
        if (kind.maybe_body.has_value()) {
          buf << indent_str(kind.maybe_body.value()->to_string(), "\t")
              << std::endl;
        }
      },
      [&buf](const stmt::Block& kind) {
        buf << "Block" << std::endl;
        buf << indent_str(kind.symtable->to_string(), "\t") << std::endl
            << std::endl;
        for (auto stmt : kind.stmts) {
          buf << indent_str(stmt->to_string(), "\t") << std::endl;
        }
      },
      // unreachable.
      [&buf](const auto& kind) { buf << "UNREACHABLE_STMT"; },
    },
    this->kind
  );

  buf << std::endl;

  return buf.str();
}

std::string Compunit::to_string() const {
  std::stringstream buf;
  buf << "Compunit" << std::endl;

  buf << indent_str(this->symtable->to_string(), "\t") << std::endl
      << std::endl;

  for (auto stmt : this->stmts) {
    buf << indent_str(stmt->to_string(), "\t") << std::endl;
  }
  return buf.str();
}

}  // namespace ast

}  // namespace frontend
}  // namespace syc