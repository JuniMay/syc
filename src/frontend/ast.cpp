#include "frontend/ast.h"
#include "frontend/driver.h"
#include "frontend/symtable.h"
#include "frontend/type.h"

namespace syc {
namespace frontend {

namespace ast {

bool Expr::operator==(const Expr& other) const {
  return this->to_string() == other.to_string();
}

std::pair<int, ExprPtr> as_integer_mul(ExprPtr expr) {
  auto maybe_binary = expr->as_binary();

  if (!maybe_binary.has_value()) {
    return {1, expr};
  }

  auto binary = maybe_binary.value();

  auto op = binary.op;

  if (op != BinaryOp::Mul) {
    return {1, expr};
  }

  auto lhs = binary.lhs;
  auto rhs = binary.rhs;

  // constant * rhs
  auto maybe_lhs_value = lhs->get_comptime_value();

  if (!maybe_lhs_value.has_value()) {
    return {1, expr};
  }

  auto lhs_value = maybe_lhs_value.value();

  if (!lhs_value->type->is_int()) {
    return {1, expr};
  }

  auto lhs_int = std::get<int>(lhs_value->kind);

  return {lhs_int, rhs};
}

void expr::InitializerList::add_expr(ExprPtr expr) {
  this->init_list.push_back(expr);
}

void expr::InitializerList::set_type(TypePtr type, Driver& driver) {
  if (!type->is_array()) {
    throw std::runtime_error("Error: non-array type for initializer list.");
  }
  if (this->init_list.size() == 0) {
    this->type = type;
    this->is_zeroinitializer = true;
  } else {
    this->type = type;
    this->is_zeroinitializer = false;

    auto length = std::get<type::Array>(type->kind).length;

    auto root_element_type = type->get_root_element_type().value();
    auto element_type = type->get_element_type().value();

    if (root_element_type == element_type) {
      // check the element type
      for (size_t i = 0; i < this->init_list.size(); i++) {
        if (this->init_list[i]->get_type() != element_type) {
          this->init_list[i] =
            create_cast_expr(this->init_list[i], element_type, driver);
        }
      }
      if (this->init_list.size() < length) {
        for (size_t i = this->init_list.size(); i < length; i++) {
          this->init_list.push_back(
            create_constant_expr(create_zero_comptime_value(element_type))
          );
        }
      }
    } else {
      // element number of the sub-array element.
      size_t element_total_length =
        element_type->get_size() / root_element_type->get_size();

      size_t idx = 0;

      // normalized init_list according to the shape of the type.
      std::vector<ExprPtr> new_init_list;
      // for creating the sub-array initializer list
      std::vector<ExprPtr> element_init_list;

      while (idx < this->init_list.size()) {
        auto expr = this->init_list[idx];

        if (expr->is_initializer_list()) {
          if (element_init_list.size() > 0) {
            element_init_list.push_back(expr);
          } else {
            // If the expression is a initializer list, just set the type.
            std::get<expr::InitializerList>(expr->kind)
              .set_type(element_type, driver);
            new_init_list.push_back(expr);
          }
        } else if (expr->get_type() == root_element_type) {
          // If the expression is a root element, add to the sub-array
          // initializer list.
          element_init_list.push_back(expr);
          // If the sub-array initializer list is full, create a element
          // initializer list expression from it.
          if (element_init_list.size() == element_total_length) {
            auto init_expr = create_initializer_list_expr(element_init_list);
            std::get<expr::InitializerList>(init_expr->kind)
              .set_type(element_type, driver);
            new_init_list.push_back(init_expr);
            element_init_list.clear();
          }
        }
        idx++;
      }
      // the element_init_list is not full.
      if (element_init_list.size() > 0) {
        auto init_expr = create_initializer_list_expr(element_init_list);
        std::get<expr::InitializerList>(init_expr->kind)
          .set_type(element_type, driver);
        new_init_list.push_back(init_expr);
        element_init_list.clear();
      }
      if (new_init_list.size() < length) {
        for (size_t i = new_init_list.size(); i < length; i++) {
          auto init_expr = create_initializer_list_expr({});
          std::get<expr::InitializerList>(init_expr->kind)
            .set_type(element_type, driver);
          new_init_list.push_back(init_expr);
        }
      }
      this->init_list = new_init_list;
    }
  }
}

bool Expr::is_comptime() const {
  return std::visit(
    overloaded{
      [](const expr::Binary& kind) {
        return kind.lhs->is_comptime() && kind.rhs->is_comptime();
      },
      [](const expr::Unary& kind) { return kind.expr->is_comptime(); },
      [](const expr::Cast& kind) { return kind.expr->is_comptime(); },
      [](const expr::Constant& kind) { return true; },
      [this](const expr::Identifier& kind) {
        if (kind.symbol->is_const) {
          return true;
        } else {
          return false;
        }
      },
      [](const expr::InitializerList& kind) {
        for (auto expr : kind.init_list) {
          if (!expr->is_comptime()) {
            return false;
          }
        }
        return true;
      },
      [](const auto&) { return false; },
    },
    this->kind
  );
}

std::optional<expr::Binary> Expr::as_binary() const {
  if (!std::holds_alternative<expr::Binary>(this->kind)) {
    return std::nullopt;
  }

  return std::get<expr::Binary>(this->kind);
}

std::optional<ComptimeValuePtr> Expr::get_comptime_value() const {
  if (!this->is_comptime()) {
    return std::nullopt;
  }

  return std::visit(
    overloaded{
      [](const expr::Binary& kind) -> std::optional<ComptimeValuePtr> {
        auto lhs = kind.lhs->get_comptime_value();
        auto rhs = kind.rhs->get_comptime_value();
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        return std::make_optional(
          comptime_compute_binary(kind.op, lhs.value(), rhs.value())
        );
      },
      [](const expr::Unary& kind) -> std::optional<ComptimeValuePtr> {
        auto expr = kind.expr->get_comptime_value();
        if (!expr.has_value()) {
          return std::nullopt;
        }
        return std::make_optional(comptime_compute_unary(kind.op, expr.value())
        );
      },
      [](const expr::Cast& kind) -> std::optional<ComptimeValuePtr> {
        auto expr = kind.expr->get_comptime_value();
        if (!expr.has_value()) {
          return std::nullopt;
        }
        return std::make_optional(comptime_compute_cast(expr.value(), kind.type)
        );
      },
      [](const expr::Constant& kind) -> std::optional<ComptimeValuePtr> {
        return std::make_optional(kind.value);
      },
      [this](const expr::Identifier& kind) -> std::optional<ComptimeValuePtr> {
        return kind.symbol->maybe_value;
      },
      [](const expr::InitializerList& kind) -> std::optional<ComptimeValuePtr> {
        if (kind.is_zeroinitializer) {
          return create_comptime_value(Zeroinitializer{}, kind.type);
        }
        std::vector<ComptimeValuePtr> init_value_list;
        for (auto expr : kind.init_list) {
          init_value_list.push_back(expr->get_comptime_value().value());
        }
        return create_comptime_value(init_value_list, kind.type);
      },
      [](const auto&) -> std::optional<ComptimeValuePtr> {
        return std::nullopt;
      },
    },
    this->kind
  );
}

TypePtr Expr::get_type() const {
  return std::visit(
    overloaded{
      [](const expr::Identifier& kind) { return kind.symbol->type; },
      [](const expr::Binary& kind) { return kind.symbol->type; },
      [](const expr::Unary& kind) { return kind.symbol->type; },
      [](const expr::Cast& kind) { return kind.symbol->type; },
      [](const expr::Constant& kind) { return kind.value->type; },
      [](const expr::InitializerList& kind) { return kind.type; },
      [](const expr::Call& kind) { return kind.symbol->type; },
    },
    this->kind
  );
}

bool Expr::is_initializer_list() const {
  return std::holds_alternative<expr::InitializerList>(this->kind);
}

std::optional<stmt::Block> Stmt::as_block() const {
  if (!std::holds_alternative<stmt::Block>(this->kind)) {
    return std::nullopt;
  }
  return std::get<stmt::Block>(this->kind);
}

Compunit::Compunit() : symtable(create_symbol_table(std::nullopt)), stmts({}) {}

ExprPtr create_initializer_list_expr(std::vector<ExprPtr> init_list) {
  return std::make_shared<Expr>(ExprKind(expr::InitializerList{
    init_list, nullptr}));
}

ExprPtr create_identifier_expr(SymbolEntryPtr symbol_entry) {
  return std::make_shared<Expr>(ExprKind(expr::Identifier{
    symbol_entry->name, symbol_entry}));
}

ExprPtr create_constant_expr(ComptimeValuePtr value) {
  return std::make_shared<Expr>(ExprKind(expr::Constant{value}));
}

ExprPtr
create_binary_expr(BinaryOp op, ExprPtr lhs, ExprPtr rhs, Driver& driver) {
  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;

  std::optional<TypePtr> type = std::nullopt;

  switch (op) {
    case BinaryOp::Add:
    case BinaryOp::Sub:
    case BinaryOp::Mul:
    case BinaryOp::Div: {
      // int          int        ->  int
      // float        float      ->  float
      // int=>float   float      ->  float
      // bool=>float  float      ->  float
      // bool=>int    int        ->  int
      // bool=>int    bool=>int  ->  int
      if (lhs->get_type()->is_bool() && rhs->get_type()->is_bool()) {
        type = create_int_type();
        lhs = create_cast_expr(lhs, type.value(), driver);
        rhs = create_cast_expr(rhs, type.value(), driver);
      } else if (lhs->get_type()->is_bool() && rhs->get_type()->is_int()) {
        type = create_int_type();
        lhs = create_cast_expr(lhs, type.value(), driver);
      } else if (lhs->get_type()->is_int() && rhs->get_type()->is_bool()) {
        type = create_int_type();
        rhs = create_cast_expr(rhs, type.value(), driver);
      } else if (lhs->get_type()->is_bool() && rhs->get_type()->is_float()) {
        type = create_float_type();
        lhs = create_cast_expr(rhs, type.value(), driver);
      } else if (lhs->get_type()->is_float() && rhs->get_type()->is_bool()) {
        type = create_float_type();
        rhs = create_cast_expr(rhs, type.value(), driver);
      } else if (lhs->get_type() == rhs->get_type()) {
        type = lhs->get_type();
      } else if (lhs->get_type()->is_float() && rhs->get_type()->is_int()) {
        type = create_float_type();
        rhs = create_cast_expr(rhs, type.value(), driver);
      } else if (lhs->get_type()->is_int() && rhs->get_type()->is_float()) {
        type = create_float_type();
        lhs = create_cast_expr(lhs, type.value(), driver);
      } else {
        std::string error_message =
          "Error: type mismatch for binary expression between " +
          lhs->get_type()->to_string() + " and " +
          rhs->get_type()->to_string() + ".";
        throw std::runtime_error(error_message);
      }
      break;
    }
    case BinaryOp::Lt:
    case BinaryOp::Gt:
    case BinaryOp::Le:
    case BinaryOp::Ge:
    case BinaryOp::Eq:
    case BinaryOp::Ne: {
      // bool         bool   ->  bool
      // int          int    ->  bool
      // float        float  ->  bool
      // int=>float   float  ->  bool
      // int=>bool    bool   ->  bool
      // float=>bool  bool   ->  bool
      if (lhs->get_type() == rhs->get_type()) {
        type = create_bool_type();
      } else if (lhs->get_type()->is_float() && rhs->get_type()->is_int()) {
        type = create_bool_type();
        rhs = create_cast_expr(rhs, create_float_type(), driver);
      } else if (lhs->get_type()->is_int() && rhs->get_type()->is_float()) {
        type = create_bool_type();
        lhs = create_cast_expr(lhs, create_float_type(), driver);
      } else if (lhs->get_type()->is_bool() && (rhs->get_type()->is_int() || rhs->get_type()->is_float())) {
        type = create_bool_type();
        rhs = create_cast_expr(rhs, create_bool_type(), driver);
      } else if (rhs->get_type()->is_bool() && (lhs->get_type()->is_int() || lhs->get_type()->is_float())) {
        type = create_bool_type();
        lhs = create_cast_expr(lhs, create_bool_type(), driver);
      } else {
        std::string error_message =
          "Error: type mismatch for logical binary expression between " +
          lhs->get_type()->to_string() + " and " +
          rhs->get_type()->to_string() + ".";
        throw std::runtime_error(error_message);
      }
      break;
    }
    case BinaryOp::Mod: {
      if (!(lhs->get_type()->is_int() && rhs->get_type()->is_int())) {
        std::string error_message =
          "Error: modulo expression only permitted on integers, not " +
          lhs->get_type()->to_string() + " and " +
          rhs->get_type()->to_string() + ".";
        throw std::runtime_error(error_message);
      }
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
      } else if (lhs->get_type()->is_pointer()) {
        type = lhs->get_type()->get_value_type();
      } else {
        throw std::runtime_error("Error: indexing on non-array type.");
      }
      if (!rhs->get_type()->is_int()) {
        rhs = create_cast_expr(rhs, create_int_type(), driver);
      }
      break;
    }
  }

  if (!type.has_value()) {
    throw std::runtime_error(
      "Error: something went wrong when checking type for binary expression."
    );
  }

  if (op == BinaryOp::Add) {
    auto integer_mul_lhs = as_integer_mul(lhs);
    auto integer_mul_rhs = as_integer_mul(rhs);

    auto lhs_int = integer_mul_lhs.first;
    auto rhs_int = integer_mul_rhs.first;
    auto lhs_expr = integer_mul_lhs.second;
    auto rhs_expr = integer_mul_rhs.second;

    std::cout << "ADD EXPR!" << std::endl;

    if (lhs_expr->to_string() == rhs_expr->to_string()) {
      std::cout << "ADD EXPR SAME!" << std::endl;
      auto new_int = create_comptime_value(lhs_int + rhs_int, type.value());
      return create_binary_expr(
        BinaryOp::Mul, create_constant_expr(new_int), lhs_expr, driver
      );
    }
  }

  auto symbol_entry = create_symbol_entry(
    Scope::Temp, symbol_name, type.value(), false, std::nullopt
  );

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(ExprKind(expr::Binary{
    op, lhs, rhs, symbol_entry}));
}

ExprPtr create_unary_expr(UnaryOp op, ExprPtr expr, Driver& driver) {
  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;
  auto type = expr->get_type();

  if (type->is_bool()) {
    if (op == UnaryOp::Pos || op == UnaryOp::Neg) {
      // +bool / -bool => +int / -int
      expr = create_cast_expr(expr, create_int_type(), driver);
    }
    auto symbol_entry = create_symbol_entry(
      Scope::Temp, symbol_name, create_int_type(), false, std::nullopt
    );
    symtable->add_symbol_entry(symbol_entry);
    return std::make_shared<Expr>(ExprKind(expr::Unary{op, expr, symbol_entry})
    );
  } else if (type->is_int() || type->is_float()) {
    if (op == UnaryOp::LogicalNot) {
      // expr is int/float: !expr -> expr == 0
      auto zero = create_constant_expr(create_zero_comptime_value(type));
      return create_binary_expr(BinaryOp::Eq, expr, zero, driver);
    } else {
      auto symbol_entry = create_symbol_entry(
        Scope::Temp, symbol_name, type, false, std::nullopt
      );
      symtable->add_symbol_entry(symbol_entry);
      return std::make_shared<Expr>(ExprKind(expr::Unary{op, expr, symbol_entry}
      ));
    }
  } else {
    throw std::runtime_error(
      "Error: unary expression on non-numeric type" + type->to_string() + "."
    );
  }
}

ExprPtr create_call_expr(
  SymbolEntryPtr func_symbol,
  std::vector<ExprPtr> args,
  Driver& driver
) {
  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;

  auto func_name = func_symbol->name;
  auto func_type = func_symbol->type;

  auto ret_type = std::get<type::Function>(func_type->kind).ret_type;
  auto param_types = std::get<type::Function>(func_type->kind).param_types;

  if (args.size() != param_types.size()) {
    std::string error_message = "Error: incompatible number of arguments.";
    throw std::runtime_error(error_message);
  }

  for (size_t i = 0; i < args.size(); i++) {
    if (param_types[i]->is_array() || param_types[i]->is_pointer()) {
      // Ref/Deref ops are not supported here so array->pointer/pointer->pointer
      // cast is carried out when generating ir.
      continue;
    }
    if (args[i]->get_type() != param_types[i]) {
      args[i] = create_cast_expr(args[i], param_types[i], driver);
    }
  }

  auto symbol_entry = create_symbol_entry(
    Scope::Temp, symbol_name, ret_type, false, std::nullopt
  );

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(ExprKind(expr::Call{
    func_symbol, args, symbol_entry}));
}

ExprPtr create_cast_expr(ExprPtr expr, TypePtr type, Driver& driver) {
  // int/float -> bool cast
  if (type->is_bool()) {
    if (expr->get_type()->is_float() || expr->get_type()->is_int()) {
      auto zero =
        create_constant_expr(create_zero_comptime_value(expr->get_type()));
      return create_binary_expr(BinaryOp::Ne, expr, zero, driver);
    } else {
      throw std::runtime_error("Error: cannot cast non-numeric type to bool.");
    }
  }

  // int/float/bool -> int/float cast
  auto symbol_name = driver.get_next_temp_name();
  auto symtable = driver.curr_symtable;

  auto symbol_entry =
    create_symbol_entry(Scope::Temp, symbol_name, type, false, std::nullopt);

  symtable->add_symbol_entry(symbol_entry);

  return std::make_shared<Expr>(ExprKind(expr::Cast{expr, type, symbol_entry}));
}

StmtPtr create_blank_stmt() {
  return std::make_shared<Stmt>(StmtKind(stmt::Blank{}));
}

StmtPtr create_return_stmt(std::optional<ExprPtr> maybe_expr, Driver& driver) {
  if (maybe_expr.has_value()) {
    auto expr = maybe_expr.value();
    auto curr_ret_type = std::get<stmt::FuncDef>(driver.curr_function->kind)
                           .symbol_entry->type->get_ret_type()
                           .value();
    if (expr->get_type() != curr_ret_type) {
      expr = create_cast_expr(expr, curr_ret_type, driver);
    }
    maybe_expr = std::make_optional(expr);
  }
  return std::make_shared<Stmt>(StmtKind(stmt::Return{maybe_expr}));
}

StmtPtr create_break_stmt() {
  return std::make_shared<Stmt>(StmtKind(stmt::Break{}));
}

StmtPtr create_continue_stmt() {
  return std::make_shared<Stmt>(StmtKind(stmt::Continue{}));
}

StmtPtr create_if_stmt(
  ExprPtr cond,
  StmtPtr then_stmt,
  Driver& driver,
  std::optional<StmtPtr> maybe_else_stmt
) {
  // cast to bool
  if (!cond->get_type()->is_bool()) {
    cond = create_cast_expr(cond, create_bool_type(), driver);
  }
  return std::make_shared<Stmt>(StmtKind(stmt::If{
    cond, then_stmt, maybe_else_stmt}));
}

StmtPtr create_while_stmt(ExprPtr cond, StmtPtr body, Driver& driver) {
  // cast to bool
  if (!cond->get_type()->is_bool()) {
    cond = create_cast_expr(cond, create_bool_type(), driver);
  }
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
    if (type->is_array()) {
      // pass array as pointer
      auto element_type = type->get_element_type().value();
      type = create_pointer_type(element_type);
    }
    param_types.push_back(type);
    param_names.push_back(name);
  }

  auto func_type = create_function_type(ret_type, param_types);

  auto symtable = create_symbol_table(parent_symtable);

  auto symbol_entry =
    create_symbol_entry(Scope::Global, name, func_type, false, std::nullopt);

  parent_symtable->add_symbol_entry(symbol_entry);

  for (auto [type, name] : params) {
    if (type->is_array()) {
      // pass array as pointer
      auto element_type = type->get_element_type().value();
      type = create_pointer_type(element_type);
    }
    auto symbol_entry =
      create_symbol_entry(Scope::Param, name, type, false, std::nullopt);
    symtable->add_symbol_entry(symbol_entry);
  }

  return std::make_shared<Stmt>(StmtKind(stmt::FuncDef{
    symtable,
    symbol_entry,
    param_names,
    std::nullopt,
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
  return std::make_shared<Stmt>(StmtKind(stmt::Expr{expr}));
}

StmtPtr create_assign_stmt(ExprPtr lhs, ExprPtr rhs, Driver& driver) {
  if (rhs->get_type() != lhs->get_type()) {
    rhs = create_cast_expr(rhs, lhs->get_type(), driver);
  }
  return std::make_shared<Stmt>(StmtKind(stmt::Assign{lhs, rhs}));
}

SymbolEntryPtr create_symbol_entry_from_decl_def(
  Scope scope,
  bool is_const,
  std::tuple<TypePtr, std::string, std::optional<ExprPtr>> def
) {
  auto [type, name, maybe_init] = def;
  std::optional<ComptimeValuePtr> maybe_value = std::nullopt;

  if (is_const) {
    if (maybe_init.has_value()) {
      maybe_value = maybe_init.value()->get_comptime_value();
    } else {
      maybe_value = create_zero_comptime_value(type);
    }
  }

  return create_symbol_entry(scope, name, type, is_const, maybe_value);
}

void stmt::Block::add_stmt(StmtPtr stmt) {
  this->stmts.push_back(stmt);
}

void stmt::FuncDef::set_body(StmtPtr body) {
  if (!std::holds_alternative<stmt::Block>(body->kind)) {
    throw std::runtime_error("Only block statement is allowed in function body."
    );
  }

  this->maybe_body = std::make_optional(body);
}

void Compunit::add_stmt(StmtPtr stmt) {
  this->stmts.push_back(stmt);
}

Expr::Expr(ExprKind kind) : kind(kind) {}

Stmt::Stmt(StmtKind kind) : kind(kind) {}

}  // namespace ast

}  // namespace frontend

}  // namespace syc