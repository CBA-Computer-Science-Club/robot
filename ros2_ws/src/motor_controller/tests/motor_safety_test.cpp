#include "motor_safety.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
  using namespace motor_safety;
  assert(!may_drive(false, false, true, true, true, true));
  assert(!may_drive(true, true, true, true, true, true));
  assert(!may_drive(true, false, false, true, true, true));
  assert(!may_drive(true, false, true, false, true, true));
  assert(!may_drive(true, false, true, true, false, true));
  assert(!may_drive(true, false, true, true, true, false));
  assert(may_drive(true, false, true, true, true, true));
  auto straight = mix(0.2, 0.0, 0.4, 0.4);
  assert(straight && std::abs(straight->first - 0.2) < 1e-8 && std::abs(straight->second - 0.2) < 1e-8);
  auto left_turn = mix(0.0, 1.0, 0.4, 0.4);
  assert(left_turn && std::abs(left_turn->first + 0.2) < 1e-8 && std::abs(left_turn->second - 0.2) < 1e-8);
  assert(!mix(0.5, 0, 0.4, 0.4));
  assert(!mix(NAN, 0, 0.4, 0.4));
  assert(!mix(0.0, INFINITY, 0.4, 0.4));
  assert(!mix(0.1, 0, -0.4, 0.4));
  std::cout << "motor_safety_test: PASS\n";
}
