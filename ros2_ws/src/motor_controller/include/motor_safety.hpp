#pragma once

#include <cmath>
#include <optional>
#include <utility>

namespace motor_safety {

inline bool may_drive(bool armed, bool emergency_latched, bool scan_clear,
                      bool command_fresh, bool scan_fresh, bool odom_fresh) {
  return armed && !emergency_latched && scan_clear &&
         command_fresh && scan_fresh && odom_fresh;
}

// Velocity mixing for a two-wheel differential base. Reject infeasible commands
// rather than masking a mismatched wheel-geometry or acceleration request.
inline std::optional<std::pair<double, double>> mix(double linear, double angular,
                                                     double separation, double max_speed) {
  if (!std::isfinite(linear) || !std::isfinite(angular) ||
      !std::isfinite(separation) || !std::isfinite(max_speed) ||
      separation <= 0 || max_speed <= 0) return std::nullopt;
  const double left = linear - angular * separation / 2.0;
  const double right = linear + angular * separation / 2.0;
  if (!std::isfinite(left) || !std::isfinite(right) ||
      std::abs(left) > max_speed || std::abs(right) > max_speed) return std::nullopt;
  return std::pair<double, double>{left, right};
}

}  // namespace motor_safety
