#ifndef SYC_FRONTEND_SYMTABLE_H_
#define SYC_FRONTEND_SYMTABLE_H_

#include "common.h"
#include "frontend/ast.h"
#include "frontend/comptime.h"

namespace syc {
namespace frontend {

/// Scope.
enum class Scope {
  /// Global variable/constant
  Global,
  /// Parameter
  Param,
  /// Local variable/constant
  Local,
  /// Temporary symbol
  Temp,
};

/// Symbol entry
struct SymbolEntry {
  /// Kind of the symbol.
  Scope scope;
  /// Name of the symbol.
  std::string name;
  /// Type of the symbol.
  TypePtr type;
  /// If the symbol is a constant value.
  /// For functions and variables, `is_const` is false.
  bool is_const;
  /// (Optional) Compile-time value of the symbol.
  /// If the symbol is a constant, the value must be given.
  std::optional<ComptimeValuePtr> maybe_value;

  /// (Optional) IR operand ID of the symbol.
  /// This is set during the IR generation phase.
  std::optional<ir::OperandID> maybe_ir_operand_id = std::nullopt;

  /// Constructor
  SymbolEntry(
    Scope scope,
    std::string name,
    TypePtr type,
    bool is_const,
    std::optional<ComptimeValuePtr> maybe_value
  );

  bool has_ir_operand() const;

  bool is_comptime() const;

  std::string to_string() const;
};

/// Symbol Table
/// A symbol table is attached to the block, function or global context.
struct SymbolTable {
  /// Table, indexed by the name.
  std::map<std::string, SymbolEntryPtr> table;
  /// The parent table.
  /// The hierarchy can be resolved while traversing the AST.
  /// `parent` here is used for looking up the symbol.
  std::optional<SymbolTablePtr> maybe_parent;

  /// Lookup a symbol in the table by its name.
  /// If the symbol is not found in the current table, the parent table will be
  /// searched. Return nullopt if no symbol is found.
  std::optional<SymbolEntryPtr> lookup(
    const std::string& name, bool lookup_parent = true
  );

  /// Add a symbol entry into the symbol table.
  void add_symbol_entry(SymbolEntryPtr symbol_entry);

  std::string to_string() const;
};

/// Create a new symbol entry.
SymbolEntryPtr create_symbol_entry(
  Scope scope,
  std::string name,
  TypePtr type,
  bool is_const,
  std::optional<ComptimeValuePtr> value
);

/// Create a new symbol table.
SymbolTablePtr create_symbol_table(
  std::optional<SymbolTablePtr> maybe_parent = std::nullopt
);

}  // namespace frontend
}  // namespace syc

#endif