#include "frontend/ast.h"
#include "frontend/driver.h"
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

TypePtr Expr::get_type() const {
  if (this->symbol_entry != nullptr) {
    return this->symbol_entry->type;
  } else {
    if (std::holds_alternative<expr::Constant>(this->kind)) {
      return std::get<expr::Constant>(this->kind).value.type;
    } else {
      return nullptr;
    }
  }
}

Compunit::Compunit() : symtable(create_symbol_table(nullptr)), stmts({}) {}

ExprPtr create_initializer_list_expr(std::vector<ExprPtr> init_list) {
  return std::make_shared<Expr>(
    ExprKind(expr::InitializerList{init_list}), nullptr
  );
}

ExprPtr create_identifier_expr(SymbolEntryPtr symbol_entry) {
  if (symbol_entry == nullptr) {
    return nullptr;
  }
  return std::make_shared<Expr>(
    ExprKind(expr::Identifier{symbol_entry->name}), symbol_entry
  );
}

ExprPtr create_constant_expr(ComptimeValue value) {
  return std::make_shared<Expr>(ExprKind(expr::Constant{value}), nullptr);
}

ExprPtr
create_binary_expr(BinaryOp op, ExprPtr lhs, ExprPtr rhs, Driver& driver) {
  if (lhs == nullptr || rhs == nullptr) {
    return nullptr;
  }
  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;

  TypePtr type = nullptr;

  // TODO: unify type checking
  switch (op) {
    case BinaryOp::Add:
    case BinaryOp::Sub:
    case BinaryOp::Mul:
    case BinaryOp::Div:
    case BinaryOp::Lt:
    case BinaryOp::Gt:
    case BinaryOp::Le:
    case BinaryOp::Ge:
    case BinaryOp::Eq:
    case BinaryOp::Ne: {
      if (lhs->get_type() == rhs->get_type()) {
        type = lhs->get_type();
      } else if (lhs->get_type()->is_float() && rhs->get_type()->is_int()) {
        type = create_float_type();
        rhs = create_cast_expr(rhs, type, driver);
      } else if (lhs->get_type()->is_int() && rhs->get_type()->is_float()) {
        type = create_float_type();
        lhs = create_cast_expr(lhs, type, driver);
      } else {
        std::cerr << "Error: type mismatch: binary expression: " << std::endl;
        return nullptr;
      }
      break;
    }
    case BinaryOp::Mod: {
      type = create_int_type();
      break;
    }
    case BinaryOp::LogicalAnd:
    case BinaryOp::LogicalOr: {
      type = create_bool_type();
      if (!lhs->get_type()->is_bool()) {
        lhs = create_cast_expr(lhs, create_bool_type(), driver);
      }
      if (!rhs->get_type()->is_bool()) {
        rhs = create_cast_expr(rhs, create_bool_type(), driver);
      }
      break;
    }
    case BinaryOp::Index: {
      if (lhs->get_type()->is_array()) {
        type = lhs->get_type()->get_element_type();
      }
      if (!rhs->get_type()->is_int()) {
        rhs = create_cast_expr(rhs, create_int_type(), driver);
      }
      break;
    }
  }

  auto symbol_entry =
    create_symbol_entry(Scope::Temp, symbol_name, type, false, std::nullopt);

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(
    ExprKind(expr::Binary{op, lhs, rhs}), symbol_entry
  );
}

ExprPtr create_unary_expr(UnaryOp op, ExprPtr expr, Driver& driver) {
  if (expr == nullptr) {
    return nullptr;
  }

  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;

  auto type = expr->symbol_entry->type;

  if (op == UnaryOp::Pos || 
      op == UnaryOp::Neg || (op == UnaryOp::LogicalNot && type->is_bool())) {
    auto symbol_entry =
      create_symbol_entry(Scope::Temp, symbol_name, type, false, std::nullopt);
    symtable->add_symbol_entry(symbol_entry);
    return std::make_shared<Expr>(
      ExprKind(expr::Unary{op, expr}), symbol_entry
    );
  } else {
    // expr is int/float: !expr -> expr == 0
    auto zero = create_constant_expr(create_zero_comptime_value(type));
    return create_binary_expr(BinaryOp::Eq, expr, zero, driver);
  }
}

ExprPtr create_call_expr(
  SymbolEntryPtr func_symbol_entry,
  std::vector<ExprPtr> args,
  Driver& driver
) {
  if (func_symbol_entry == nullptr) {
    return nullptr;
  }

  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;

  auto func_name = func_symbol_entry->name;
  auto func_type = func_symbol_entry->type;

  auto ret_type = std::get<type::Function>(func_type->kind).ret_type;

  auto symbol_entry = create_symbol_entry(
    Scope::Temp, symbol_name, ret_type, false, std::nullopt
  );

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(
    ExprKind(expr::Call{func_name, args}), symbol_entry
  );
}

ExprPtr create_cast_expr(ExprPtr expr, TypePtr type, Driver& driver) {
  if (expr == nullptr || type == nullptr) {
    return nullptr;
  }

  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;

  auto symbol_entry =
    create_symbol_entry(Scope::Temp, symbol_name, type, false, std::nullopt);

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(ExprKind(expr::Cast{expr, type}), symbol_entry);
}

StmtPtr create_blank_stmt() {
  return std::make_shared<Stmt>(StmtKind(stmt::Blank{}));
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

StmtPtr create_decl_stmt(
  Scope scope,
  bool is_const,
  std::vector<std::tuple<TypePtr, std::string, std::optional<ExprPtr>>> defs
) {
  return std::make_shared<Stmt>(StmtKind(stmt::Decl{scope, is_const, defs}));
}

StmtPtr create_expr_stmt(ExprPtr expr) {
  if (expr == nullptr) {
    return create_blank_stmt();
  }
  return std::make_shared<Stmt>(StmtKind(stmt::Expr{expr}));
}

StmtPtr create_assign_stmt(ExprPtr lhs, ExprPtr rhs) {
  if (lhs == nullptr || rhs == nullptr) {
    return create_blank_stmt();
  }

  return std::make_shared<Stmt>(StmtKind(stmt::Assign{lhs, rhs}));
}

void stmt::Block::add_stmt(StmtPtr stmt) {
  this->stmts.push_back(stmt);

  std::visit(
    overloaded{
      [this](const stmt::Decl& decl) {
        size_t def_cnt = decl.get_def_cnt();
        for (size_t i = 0; i < def_cnt; i++) {
          auto entry = decl.fetch_symbol_entry(i);
          this->symtable->add_symbol_entry(entry);
        }
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
        size_t def_cnt = decl.get_def_cnt();
        for (size_t i = 0; i < def_cnt; i++) {
          auto entry = decl.fetch_symbol_entry(i);
          this->symtable->add_symbol_entry(entry);
        }
      },
      [](const auto& others) {
        // do nothing
      },
    },
    stmt->kind
  );
}

size_t stmt::Decl::get_def_cnt() const {
  return this->defs.size();
}

SymbolEntryPtr stmt::Decl::fetch_symbol_entry(size_t idx) const {
  auto [type, name, maybe_init] = this->defs.at(idx);

  std::optional<ComptimeValue> value = std::nullopt;

  if (this->is_const) {
    if (maybe_init.has_value()) {
      auto init_expr = maybe_init.value();
      if (std::holds_alternative<expr::Constant>(init_expr->kind)) {
        value = std::get<expr::Constant>(init_expr->kind).value;
      }
    } else {
      value = create_zero_comptime_value(type);
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