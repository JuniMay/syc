#ifndef SYC_FRONTEND_SYMTABLE_H_
#define SYC_FRONTEND_SYMTABLE_H_

#include "common.h"

namespace syc {
namespace frontend {

/// Symbol kind
enum class SymbolKind {
  /// Global variable/constant
  Global,
  /// Parameter
  Param,
  /// Local variable/constant
  Local,
};

/// Symbol entry
struct SymbolEntry {
  /// Kind of the symbol.
  SymbolKind kind;
  /// Name of the symbol.
  std::string name;
  /// Type of the symbol.
  TypePtr type;
  /// If the symbol is a constant value.
  /// For functions and variables,  `is_const` <- false.
  bool is_const;
  /// (Optional) Compile-time value of the symbol.
  std::optional<ComptimeValue> value;
};

/// Symbol Table
/// A symbol table is attached to the block, function or global context.
struct SymbolTable {
  /// Table, indexed by the name.
  std::map<std::string, SymbolEntryPtr> table;
  /// The parent table.
  /// The hierarchy can be resolved while traversing the AST.
  /// `parent` here is used for looking up the symbol.
  SymbolTablePtr parent;

  /// Lookup a symbol in the table by its name.
  /// If the symbol is not found in the current table, the parent table will be
  /// searched. Return nullptr if no symbol is found.
  SymbolEntryPtr lookup(const std::string& name);

  /// Add a symbol entry into the symbol table.
  void add_symbol_entry(SymbolEntryPtr symbol_entry);
};

/// Create a new symbol table.
SymbolTablePtr create_symbol_table(SymbolTablePtr parent = nullptr);

}  // namespace frontend
}  // namespace syc

#endif