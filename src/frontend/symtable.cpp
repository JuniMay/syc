#include "frontend/symtable.h"
#include "frontend/comptime.h"

namespace syc {
namespace frontend {

SymbolEntry::SymbolEntry(
  Scope scope,
  std::string name,
  TypePtr type,
  bool is_const,
  std::optional<ComptimeValuePtr> maybe_value
)
  : scope(scope),
    name(name),
    type(type),
    is_const(is_const),
    maybe_value(maybe_value) {}

SymbolEntryPtr create_symbol_entry(
  Scope scope,
  std::string name,
  TypePtr type,
  bool is_const,
  std::optional<ComptimeValuePtr> value
) {
  return std::make_shared<SymbolEntry>(scope, name, type, is_const, value);
}

bool SymbolEntry::has_ir_operand() const {
  return maybe_ir_operand_id.has_value();
}

bool SymbolEntry::is_comptime() const {
  return maybe_value.has_value();
}

std::string SymbolEntry::to_string() const {
  std::stringstream ss;
  switch (scope) {
    case Scope::Global:
      ss << "GLOBAL";
      break;
    case Scope::Param:
      ss << "PARAM";
      break;
    case Scope::Local:
      ss << "LOCAL";
      break;
    case Scope::Temp:
      ss << "TEMP";
      break;
  }
  ss << " " << name << " " << type->to_string();
  if (is_const) {
    ss << " (CONST)";
  }
  if (maybe_value.has_value()) {
    ss << " = " << maybe_value.value()->to_string();
  }
  return ss.str();
}

SymbolTablePtr create_symbol_table(std::optional<SymbolTablePtr> maybe_parent) {
  auto table = std::make_shared<SymbolTable>();
  table->maybe_parent = maybe_parent;
  return table;
}

std::optional<SymbolEntryPtr>
SymbolTable::lookup(const std::string& name, bool lookup_parent) {
  auto it = table.find(name);
  if (it != table.end()) {
    return it->second;
  }
  if (lookup_parent && this->maybe_parent.has_value()) {
    return this->maybe_parent.value()->lookup(name);
  }
  return std::nullopt;
}

void SymbolTable::add_symbol_entry(SymbolEntryPtr symbol_entry) {
  table[symbol_entry->name] = symbol_entry;
}

std::string SymbolTable::to_string() const {
  std::stringstream ss;
  ss << "SymbolTable: " << std::endl;
  for (auto& [name, symbol_entry] : table) {
    ss << "- " << symbol_entry->to_string() << std::endl;
  }
  return ss.str();
}

}  // namespace frontend

}  // namespace syc