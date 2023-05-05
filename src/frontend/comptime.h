#ifndef SYC_FRONTEND_COMPTIME_H_
#define SYC_FRONTEND_COMPTIME_H_

#include "common.h"

#include "frontend/type.h"

namespace syc {
namespace frontend {

/// Compile-time value.
/// This is the representation of literal, constant, and compile time computed
/// result.
/// TODO: array?
struct ComptimeValue {
  /// The value is bool, int or float.
  std::variant<bool, int, float> value;
  /// Type of the value.
  TypePtr type;

  /// Convert to string.
  std::string to_string() const;
};

ComptimeValuePtr
create_comptime_value(std::variant<bool, int, float> value, TypePtr type);

ComptimeValuePtr create_zero_comptime_value(TypePtr type);

/// Compute binary operation between two compile-time values.
ComptimeValuePtr comptime_compute_binary(
  BinaryOp op,
  ComptimeValuePtr lhs,
  ComptimeValuePtr rhs
);

/// Compute unary operation on a compile-time value.
ComptimeValuePtr comptime_compute_unary(UnaryOp op, ComptimeValuePtr val);

/// Compute cast operation on a compile-time value.
ComptimeValuePtr comptime_compute_cast(ComptimeValuePtr val, TypePtr type);

}  // namespace frontend
}  // namespace syc

#endif