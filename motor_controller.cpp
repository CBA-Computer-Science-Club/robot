#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <lgpio.h>

class MotorController : public rclcpp::Node {
public:
    MotorController() : Node("motor_controller") {
        // 1. Initialize GPIO on Pi 5
        gpio_handle_ = lg_gpio_chip_open(0);
        
        // Define your pins (Example: BCM 18 and 23 for a simple motor)
        motor_pin_a_ = 18; 
        motor_pin_b_ = 23;
        
        lg_gpio_claim_output(gpio_handle_, 0, motor_pin_a_, 0);
        lg_gpio_claim_output(gpio_handle_, 0, motor_pin_b_, 0);

        // 2. Subscribe to "cmd_vel" topic
        subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10, std::bind(&MotorController::on_move_received, this, std::placeholders::_1));
            
        RCLCPP_INFO(this->get_logger(), "Motor Node started. Waiting for commands...");
    }

    ~MotorController() {
        lg_gpio_chip_close(gpio_handle_);
    }

private:
    void on_move_received(const geometry_msgs::msg::Twist::SharedPtr msg) {
        // Logic for Forward/Backward based on linear.x
        if (msg->linear.x > 0.1) {
            drive_forward();
        } else if (msg->linear.x < -0.1) {
            drive_backward();
        } else {
            stop_motors();
        }
    }

    void drive_forward() {
        lg_gpio_write(gpio_handle_, motor_pin_a_, 1);
        lg_gpio_write(gpio_handle_, motor_pin_b_, 0);
    }

    void drive_backward() {
        lg_gpio_write(gpio_handle_, motor_pin_a_, 0);
        lg_gpio_write(gpio_handle_, motor_pin_b_, 1);
    }

    void stop_motors() {
        lg_gpio_write(gpio_handle_, motor_pin_a_, 0);
        lg_gpio_write(gpio_handle_, motor_pin_b_, 0);
    }

    int gpio_handle_;
    int motor_pin_a_, motor_pin_b_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscription_;
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorController>());
    rclcpp::shutdown();
    return 0;
}