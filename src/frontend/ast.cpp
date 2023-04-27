#include "frontend/ast.h"
#include "frontend/symtable.h"
#include "frontend/type.h"

namespace syc {
namespace frontend {

namespace ast {

bool Expr::is_comptime() const {
  return std::visit(
    overloaded{
      [](const expr::Binary& kind) {
        return kind.lhs->is_comptime() && kind.rhs->is_comptime();
      },
      [](const expr::Unary& kind) { return kind.expr->is_comptime(); },
      [](const expr::Cast& kind) { return kind.expr->is_comptime(); },
      [](const expr::Constant& kind) { return true; },
      [](const auto&) { return false; },
    },
    this->kind
  );
}

std::optional<ComptimeValue> Expr::get_comptime_value() const {
  if (!this->is_comptime()) {
    return std::nullopt;
  }

  return std::visit(
    overloaded{
      [](const expr::Binary& kind) -> std::optional<ComptimeValue> {
        auto lhs = kind.lhs->get_comptime_value();
        auto rhs = kind.rhs->get_comptime_value();
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        return std::make_optional(
          comptime_compute_binary(kind.op, lhs.value(), rhs.value())
        );
      },
      [](const expr::Unary& kind) -> std::optional<ComptimeValue> {
        auto expr = kind.expr->get_comptime_value();
        if (!expr.has_value()) {
          return std::nullopt;
        }
        return std::make_optional(comptime_compute_unary(kind.op, expr.value())
        );
      },
      [](const expr::Cast& kind) -> std::optional<ComptimeValue> {
        auto expr = kind.expr->get_comptime_value();
        if (!expr.has_value()) {
          return std::nullopt;
        }
        return std::make_optional(comptime_compute_cast(expr.value(), kind.type)
        );
      },
      [](const expr::Constant& kind) -> std::optional<ComptimeValue> {
        return std::make_optional(kind.value);
      },
      [](const auto&) -> std::optional<ComptimeValue> { return std::nullopt; },
    },
    this->kind
  );
}

Compunit::Compunit() : symtable(create_symbol_table(nullptr)), stmts({}) {}

ExprPtr create_identifier_expr(SymbolEntryPtr symbol_entry) {
  return std::make_shared<Expr>(
    ExprKind(expr::Identifier{symbol_entry->name}), symbol_entry
  );
}

ExprPtr create_constant_expr(ComptimeValue value) {
  return std::make_shared<Expr>(ExprKind(expr::Constant{value}), nullptr);
}

ExprPtr create_binary_expr(
  BinaryOp op,
  ExprPtr lhs,
  ExprPtr rhs,
  std::string symbol_name,
  SymbolTablePtr symtable
) {
  // TODO: decide type of the result.
  auto symbol_entry =
    create_symbol_entry(Scope::Temp, symbol_name, nullptr, false, std::nullopt);

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(
    ExprKind(expr::Binary{op, lhs, rhs}), symbol_entry
  );
}

ExprPtr create_unary_expr(
  UnaryOp op,
  ExprPtr expr,
  std::string symbol_name,
  SymbolTablePtr symtable
) {
  // TODO: decide type of the result.
  auto symbol_entry =
    create_symbol_entry(Scope::Temp, symbol_name, nullptr, false, std::nullopt);

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(ExprKind(expr::Unary{op, expr}), symbol_entry);
}

ExprPtr create_call_expr(
  SymbolEntryPtr func_symbol_entry,
  std::vector<ExprPtr> args,
  std::string symbol_name,
  SymbolTablePtr symtable
) {
  auto func_name = func_symbol_entry->name;
  auto func_type = func_symbol_entry->type;
  
  auto ret_type = std::get<type::Function>(func_type->kind).ret_type;

  auto symbol_entry =
    create_symbol_entry(Scope::Temp, symbol_name, ret_type, false, std::nullopt);

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(
    ExprKind(expr::Call{func_name, args}), symbol_entry
  );
}

StmtPtr create_return_stmt(ExprPtr expr) {
  return std::make_shared<Stmt>(StmtKind(stmt::Return{expr}));
}

StmtPtr create_break_stmt() {
  return std::make_shared<Stmt>(StmtKind(stmt::Break{}));
}

StmtPtr create_continue_stmt() {
  return std::make_shared<Stmt>(StmtKind(stmt::Continue{}));
}

StmtPtr create_if_stmt(ExprPtr cond, StmtPtr then_stmt, StmtPtr else_stmt) {
  return std::make_shared<Stmt>(StmtKind(stmt::If{cond, then_stmt, else_stmt}));
}

StmtPtr create_while_stmt(ExprPtr cond, StmtPtr body) {
  return std::make_shared<Stmt>(StmtKind(stmt::While{cond, body}));
}

StmtPtr create_block_stmt(SymbolTablePtr parent_symtable) {
  auto symtable = create_symbol_table(parent_symtable);
  return std::make_shared<Stmt>(StmtKind(stmt::Block{symtable, {}}));
}

StmtPtr create_func_def_stmt(
  SymbolTablePtr parent_symtable,
  TypePtr ret_type,
  std::string name,
  std::vector<std::tuple<TypePtr, std::string>> params
) {
  std::vector<TypePtr> param_types;
  std::vector<std::string> param_names;

  for (auto [type, name] : params) {
    param_types.push_back(type);
    param_names.push_back(name);
  }

  auto func_type = create_function_type(ret_type, param_types);

  auto symtable = create_symbol_table(parent_symtable);

  auto symbol_entry =
    create_symbol_entry(Scope::Global, name, func_type, false, std::nullopt);

  parent_symtable->add_symbol_entry(symbol_entry);

  for (auto [type, name] : params) {
    auto symbol_entry =
      create_symbol_entry(Scope::Param, name, type, false, std::nullopt);
    symtable->add_symbol_entry(symbol_entry);
  }

  return std::make_shared<Stmt>(StmtKind(stmt::FuncDef{
    symtable,
    symbol_entry,
    param_names,
    nullptr,
  }));
}

void stmt::Block::add_stmt(StmtPtr stmt) {
  this->stmts.push_back(stmt);

  std::visit(
    overloaded{
      [this](const stmt::Decl& decl) {
        auto entry = decl.fetch_symbol_entry();
        this->symtable->add_symbol_entry(entry);
      },
      [](const auto& others) {
        // do nothing
      },
    },
    stmt->kind
  );
}

void stmt::FuncDef::set_body(StmtPtr body) {
  if (!std::holds_alternative<stmt::Block>(body->kind)) {
    throw std::runtime_error("Only block statement is allowed in function body."
    );
  }

  this->body = body;
}

void Compunit::add_stmt(StmtPtr stmt) {
  this->stmts.push_back(stmt);

  std::visit(
    overloaded{
      [this](const stmt::Decl& decl) {
        auto entry = decl.fetch_symbol_entry();
        this->symtable->add_symbol_entry(entry);
      },
      [](const auto& others) {
        // do nothing
      },
    },
    stmt->kind
  );
}

SymbolEntryPtr stmt::Decl::fetch_symbol_entry() const {
  std::optional<ComptimeValue> value = std::nullopt;

  // Decide if the expression is a compile-time value.
  if (this->maybe_init.has_value()) {
    auto init_expr = this->maybe_init.value();
    if (std::holds_alternative<expr::Constant>(init_expr->kind)) {
      value = std::get<expr::Constant>(init_expr->kind).value;
    }
  }

  return create_symbol_entry(scope, name, type, is_const, value);
}

Expr::Expr(ExprKind kind, SymbolEntryPtr symbol_entry)
  : kind(kind), symbol_entry(symbol_entry) {}

Stmt::Stmt(StmtKind kind) : kind(kind) {}

}  // namespace ast

}  // namespace frontend

}  // namespace syc