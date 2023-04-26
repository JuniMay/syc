#include "frontend/symtable.h"

namespace syc {
namespace frontend {

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