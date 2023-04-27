#include "frontend/symtable.h"
#include "frontend/comptime.h"

namespace syc {
namespace frontend {

SymbolEntry::SymbolEntry(
  Scope scope,
  std::string name,
  TypePtr type,
  bool is_const,
  std::optional<ComptimeValue> maybe_value
)
  : scope(scope), name(name), type(type), is_const(is_const), maybe_value(maybe_value) {}

SymbolEntryPtr create_symbol_entry(
  Scope scope,
  std::string name,
  TypePtr type,
  bool is_const,
  std::optional<ComptimeValue> value
) {
  return std::make_shared<SymbolEntry>(scope, name, type, is_const, value);
}

bool SymbolEntry::has_ir_operand() const {
  return ir_operand_id.has_value();
}

bool SymbolEntry::is_comptime() const {
  return maybe_value.has_value();
}

SymbolTablePtr create_symbol_table(SymbolTablePtr parent) {
  auto table = std::make_shared<SymbolTable>();
  table->parent = parent;
  return table;
}

SymbolEntryPtr SymbolTable::lookup(const std::string& name) {
  auto it = table.find(name);
  if (it != table.end()) {
    return it->second;
  }
  if (parent) {
    return parent->lookup(name);
  }
  return nullptr;
}

void SymbolTable::add_symbol_entry(SymbolEntryPtr symbol_entry) {
  table[symbol_entry->name] = symbol_entry;
}

}  // namespace frontend

}  // namespace syc