#ifndef SYC_COMMON_H_
#define SYC_COMMON_H_

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace syc {

namespace frontend {

namespace type {

struct Integer;
struct Float;
struct Array;
struct Void;
struct Pointer;
struct Function;

}  // namespace type

using TypeKind = std::variant<
  type::Integer,
  type::Float,
  type::Array,
  type::Void,
  type::Pointer,
  type::Function>;

struct Type;
using TypePtr = std::shared_ptr<Type>;

enum class BinaryOp;
enum class UnaryOp;

struct Zeroinitializer;
struct ComptimeValue;
using ComptimeValuePtr = std::shared_ptr<ComptimeValue>;
using ComptimeValueKind = std::
  variant<bool, int, float, std::vector<ComptimeValuePtr>, Zeroinitializer>;

enum class Scope;

struct SymbolEntry;
struct SymbolTable;

using SymbolEntryPtr = std::shared_ptr<SymbolEntry>;
using SymbolTablePtr = std::shared_ptr<SymbolTable>;

namespace ast {

namespace expr {

struct Identifier;
struct Binary;
struct Unary;
struct Call;
struct Cast;
struct Constant;
struct InitializerList;

}  // namespace expr

namespace stmt {

struct Blank;
struct If;
struct While;
struct Break;
struct Continue;
struct Return;
struct Block;
struct Assign;
struct Expr;
struct Decl;
struct FuncDef;

}  // namespace stmt

using ExprKind = std::variant<
  expr::Identifier,
  expr::Binary,
  expr::Unary,
  expr::Call,
  expr::Constant,
  expr::Cast,
  expr::InitializerList>;

struct Expr;

using StmtKind = std::variant<
  stmt::Blank,
  stmt::If,
  stmt::While,
  stmt::Break,
  stmt::Continue,
  stmt::Return,
  stmt::Assign,
  stmt::Block,
  stmt::Expr,
  stmt::Decl,
  stmt::FuncDef>;

struct Stmt;

struct Compunit;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

}  // namespace ast

struct Driver;

}  // namespace frontend

namespace ir {

using BasicBlockID = size_t;
using InstructionID = size_t;
using OperandID = size_t;

struct Context;
struct BasicBlock;
struct Function;

using BasicBlockPtr = std::shared_ptr<BasicBlock>;
using BasicBlockPrevPtr = std::weak_ptr<BasicBlock>;

using FunctionPtr = std::shared_ptr<Function>;

namespace type {

struct Void;
struct Integer;
struct Float;
struct Array;
struct Pointer;
struct Label;

};  // namespace type

using Type = std::
  variant<type::Void, type::Integer, type::Float, type::Array, type::Pointer>;

using TypePtr = std::shared_ptr<Type>;

namespace instruction {

enum class BinaryOp;
struct Binary;
enum class ICmpCond;
struct ICmp;
enum class FCmpCond;
struct FCmp;
enum class CastOp;
struct Cast;
struct Ret;
struct CondBr;
struct Br;
struct Phi;
struct Alloca;
struct Load;
struct Store;
struct GetElementPtr;
struct Call;
struct Dummy;

}  // namespace instruction

using InstructionKind = std::variant<
  instruction::Binary,
  instruction::ICmp,
  instruction::FCmp,
  instruction::Cast,
  instruction::Ret,
  instruction::CondBr,
  instruction::Br,
  instruction::Phi,
  instruction::Alloca,
  instruction::Load,
  instruction::Store,
  instruction::Call,
  instruction::GetElementPtr,
  instruction::Dummy>;

struct Instruction;

using InstructionPtr = std::shared_ptr<Instruction>;
using InstructionPrevPtr = std::weak_ptr<Instruction>;

namespace operand {

struct Global;

struct Zeroinitializer;
struct Constant;
using ConstantPtr = std::shared_ptr<Constant>;
using ConstantKind =
  std::variant<int, float, std::vector<ConstantPtr>, Zeroinitializer>;

struct Parameter;
struct Arbitrary;

}  // namespace operand

using OperandKind = std::variant<
  operand::Arbitrary,
  operand::ConstantPtr,
  operand::Parameter,
  operand::Global>;

struct Operand;

using OperandPtr = std::shared_ptr<Operand>;

}  // namespace ir

namespace backend {

using IROperandID = ir::OperandID;

enum class GPRegister;
enum class FPRegister;

struct Register;

enum class VirtualRegisterKind;

using VirtualRegisterID = size_t;

struct VirtualRegister;

struct Immediate;

using BasicBlockID = size_t;

struct BasicBlock;

using BasicBlockPtr = std::shared_ptr<BasicBlock>;
using BasicBlockPrevPtr = std::weak_ptr<BasicBlock>;

struct Function;

using FunctionPtr = std::shared_ptr<Function>;

struct Context;
namespace instruction {

struct Load;
struct FloatLoad;
struct Store;
struct FloatStore;
struct FloatMove;
struct FloatConvert;
struct Binary;
struct BinaryImm;
struct FloatBinary;
struct FloatMulAdd;
struct FloatUnary;
struct Lui;
struct Li;
struct Call;
struct Branch;
struct Dummy;

}  // namespace instruction

using InstructionKind = std::variant<
  instruction::Load,
  instruction::FloatLoad,
  instruction::Store,
  instruction::FloatStore,
  instruction::FloatMove,
  instruction::FloatConvert,
  instruction::Binary,
  instruction::BinaryImm,
  instruction::FloatBinary,
  instruction::FloatMulAdd,
  instruction::FloatUnary,
  instruction::Lui,
  instruction::Li,
  instruction::Call,
  instruction::Branch,
  instruction::Dummy>;

using InstructionID = size_t;

struct Instruction;

using InstructionPtr = std::shared_ptr<Instruction>;
using InstructionPrevPtr = std::weak_ptr<Instruction>;

struct Memory;

using OperandID = size_t;

struct Global;

using OperandKind = std::variant<Immediate, VirtualRegister, Register, Global>;

struct Operand;

using OperandPtr = std::shared_ptr<Operand>;

struct Function;

struct Builder;

}  // namespace backend

}  // namespace syc

#endif