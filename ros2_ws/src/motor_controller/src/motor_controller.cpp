#include "motor_safety.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <lgpio.h>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <std_msgs/msg/bool.hpp>

using namespace std::chrono_literals;

// GPIO controls a two-channel H-bridge. Wheel encoders/odometry and an E-stop
// circuit are external hardware; this node deliberately never fabricates odom.
class MotorController final : public rclcpp::Node {
public:
  MotorController() : Node("motor_controller") {
    armed_ = declare_parameter<bool>("armed", false);
    const int chip = declare_parameter<int>("gpio_chip", 0);
    left_forward_ = declare_parameter<int>("left_forward_pin", 18);
    left_reverse_ = declare_parameter<int>("left_reverse_pin", 23);
    right_forward_ = declare_parameter<int>("right_forward_pin", 24);
    right_reverse_ = declare_parameter<int>("right_reverse_pin", 25);
    wheel_separation_ = declare_parameter<double>("wheel_separation_m", 0.35);
    max_wheel_speed_ = declare_parameter<double>("max_wheel_speed_mps", 0.4);
    stop_distance_ = declare_parameter<double>("stop_distance_m", 0.55);
    const auto pins = std::array<int, 4>{left_forward_, left_reverse_, right_forward_, right_reverse_};
    auto sorted_pins = pins;
    std::sort(sorted_pins.begin(), sorted_pins.end());
    if (wheel_separation_ <= 0 || max_wheel_speed_ <= 0 || stop_distance_ <= 0 ||
        std::any_of(pins.begin(), pins.end(), [](int p) { return p < 0; }) ||
        std::adjacent_find(sorted_pins.begin(), sorted_pins.end()) != sorted_pins.end()) {
      throw std::invalid_argument("Invalid motor geometry or GPIO pin configuration");
    }
    gpio_ = lgGpiochipOpen(chip);
    if (gpio_ < 0) throw std::runtime_error("Cannot open lgpio chip");
    for (int pin : pins) {
      if (lgGpioClaimOutput(gpio_, 0, pin, 0) < 0) {
        stop_motors();
        lgGpiochipClose(gpio_);
        gpio_ = -1;
        throw std::runtime_error("Cannot claim all motor GPIO pins");
      }
    }
    stop_motors();
    const auto qos = rclcpp::SensorDataQoS();
    command_ = create_subscription<geometry_msgs::msg::Twist>(
        "cmd_vel", 10, std::bind(&MotorController::on_command, this, std::placeholders::_1));
    odom_ = create_subscription<nav_msgs::msg::Odometry>(
        "odom", qos, [this](nav_msgs::msg::Odometry::ConstSharedPtr) { last_odom_ = std::chrono::steady_clock::now(); });
    scan_ = create_subscription<sensor_msgs::msg::LaserScan>(
        "scan", qos, std::bind(&MotorController::on_scan, this, std::placeholders::_1));
    emergency_ = create_subscription<std_msgs::msg::Bool>(
        "emergency_stop", 10, [this](std_msgs::msg::Bool::ConstSharedPtr msg) {
          if (msg->data) { estop_latched_ = true; stop_motors(); }
        });
    watchdog_ = create_wall_timer(50ms, [this] {
      if (!motor_safety::may_drive(armed_, estop_latched_, scan_clear_,
                                   !stale(last_command_, 250ms), !stale(last_scan_, 500ms),
                                   !stale(last_odom_, 500ms))) stop_motors();
    });
    RCLCPP_WARN(get_logger(), "Motor output %s. Require live /scan, /odom and cmd_vel; physical E-stop required.",
                armed_ ? "ARMED" : "DISARMED");
  }

  ~MotorController() override {
    if (gpio_ >= 0) { stop_motors(); lgGpiochipClose(gpio_); }
  }

private:
  template<class Rep, class Period>
  bool stale(std::chrono::steady_clock::time_point at, std::chrono::duration<Rep, Period> limit) const {
    return at == std::chrono::steady_clock::time_point{} ||
           std::chrono::steady_clock::now() - at > limit;
  }
  void stop_motors() {
    if (gpio_ < 0) return;
    for (int pin : {left_forward_, left_reverse_, right_forward_, right_reverse_}) {
      lgTxPwm(gpio_, pin, 1000, 0, 0, 0);
      lgGpioWrite(gpio_, pin, 0);
    }
  }
  void drive_wheel(int forward, int reverse, double speed) {
    const int active = speed >= 0 ? forward : reverse;
    const int inactive = speed >= 0 ? reverse : forward;
    // Disable previous direction before energizing the new one (no shoot-through).
    lgTxPwm(gpio_, inactive, 1000, 0, 0, 0);
    lgGpioWrite(gpio_, inactive, 0);
    const float duty = static_cast<float>(100.0 * std::min(1.0, std::abs(speed) / max_wheel_speed_));
    if (lgTxPwm(gpio_, active, 1000, duty, 0, 0) < 0) {
      estop_latched_ = true;
      stop_motors();
    }
  }
  void on_scan(sensor_msgs::msg::LaserScan::ConstSharedPtr msg) {
    scan_clear_ = false;
    if (!std::isfinite(msg->angle_min) || !std::isfinite(msg->angle_increment) ||
        msg->angle_increment == 0) { stop_motors(); return; }
    bool has_front_measurement = false;
    for (size_t i = 0; i < msg->ranges.size(); ++i) {
      const double angle = msg->angle_min + i * msg->angle_increment;
      if (std::abs(std::atan2(std::sin(angle), std::cos(angle))) > 0.45) continue;
      const double range = msg->ranges[i];
      if (!std::isfinite(range) || range < msg->range_min || range > msg->range_max) continue;
      has_front_measurement = true;
      if (range < stop_distance_) { stop_motors(); last_scan_ = std::chrono::steady_clock::now(); return; }
    }
    scan_clear_ = has_front_measurement;
    last_scan_ = std::chrono::steady_clock::now();
    if (!scan_clear_) stop_motors();
  }
  void on_command(geometry_msgs::msg::Twist::ConstSharedPtr msg) {
    last_command_ = std::chrono::steady_clock::now();
    if (!motor_safety::may_drive(armed_, estop_latched_, scan_clear_,
                                 !stale(last_command_, 250ms), !stale(last_scan_, 500ms),
                                 !stale(last_odom_, 500ms))) {
      stop_motors(); return;
    }
    const auto wheel_speeds = motor_safety::mix(msg->linear.x, msg->angular.z,
                                                wheel_separation_, max_wheel_speed_);
    if (!wheel_speeds) { stop_motors(); return; }
    drive_wheel(left_forward_, left_reverse_, wheel_speeds->first);
    if (!estop_latched_) drive_wheel(right_forward_, right_reverse_, wheel_speeds->second);
  }
  bool armed_{false}, estop_latched_{false}, scan_clear_{false};
  int gpio_{-1}, left_forward_{0}, left_reverse_{0}, right_forward_{0}, right_reverse_{0};
  double wheel_separation_{0.35}, max_wheel_speed_{0.4}, stop_distance_{0.55};
  std::chrono::steady_clock::time_point last_command_{}, last_scan_{}, last_odom_{};
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr command_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr emergency_;
  rclcpp::TimerBase::SharedPtr watchdog_;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  try { rclcpp::spin(std::make_shared<MotorController>()); }
  catch (const std::exception &e) { RCLCPP_FATAL(rclcpp::get_logger("motor_controller"), "%s", e.what()); rclcpp::shutdown(); return 1; }
  rclcpp::shutdown();
  return 0;
}
