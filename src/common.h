#ifndef SYC_COMMON_H_
#define SYC_COMMON_H_

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <optional>
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
namespace ir {

using BasicBlockID = size_t;
using InstructionID = size_t;
using OperandID = size_t;

struct Context;
struct BasicBlock;
struct Function;

using BasicBlockPtr = std::shared_ptr<BasicBlock>;
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

}  // namespace instruction

using InstructionKind = std::variant<instruction::Binary,
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
                                     instruction::GetElementPtr>;

struct Instruction;

using InstructionPtr = std::shared_ptr<Instruction>;

namespace operand {

struct Global;
struct Immediate;
struct Parameter;
struct Arbitrary;

}  // namespace operand

using OperandKind = std::variant<operand::Arbitrary,
                                 operand::Immediate,
                                 operand::Parameter,
                                 operand::Global>;

struct Operand;

using OperandPtr = std::shared_ptr<Operand>;

}  // namespace ir
}  // namespace syc

#endif