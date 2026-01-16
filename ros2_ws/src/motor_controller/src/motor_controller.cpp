#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <lgpio.h>

class MotorController : public rclcpp::Node {
public:
    MotorController() : Node("motor_controller") {
        gpio_ = lg_gpio_chip_open(0);
        motor_pin_a_ = 18;
        motor_pin_b_ = 23;
        lg_gpio_claim_output(gpio_, 0, motor_pin_a_, 0);
        lg_gpio_claim_output(gpio_, 0, motor_pin_b_, 0);

        subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "cmd_vel", 10, std::bind(&MotorController::on_move_received, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(), "Motor Node started.");
    }

    ~MotorController() {
        if (gpio_ != 0) lg_gpio_chip_close(gpio_);
    }

private:
    void on_move_received(const geometry_msgs::msg::Twist::SharedPtr msg) {
        if (msg->linear.x > 0.1) {
            drive_forward();
        } else if (msg->linear.x < -0.1) {
            drive_backward();
        } else {
            stop_motors();
        }
    }

    void drive_forward() {
        lg_gpio_write(gpio_, motor_pin_a_, 1);
        lg_gpio_write(gpio_, motor_pin_b_, 0);
    }

    void drive_backward() {
        lg_gpio_write(gpio_, motor_pin_a_, 0);
        lg_gpio_write(gpio_, motor_pin_b_, 1);
    }

    void stop_motors() {
        lg_gpio_write(gpio_, motor_pin_a_, 0);
        lg_gpio_write(gpio_, motor_pin_b_, 0);
    }

    int gpio_;
    int motor_pin_a_, motor_pin_b_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscription_;
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotorController>());
    rclcpp::shutdown();
    return 0;
}