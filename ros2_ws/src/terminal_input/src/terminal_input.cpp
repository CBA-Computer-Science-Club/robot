#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <thread>
#include <atomic>

using namespace std::chrono_literals;

class TerminalInputNode : public rclcpp::Node {
public:
  TerminalInputNode() : Node("terminal_input") {
    pub_ = this->create_publisher<std_msgs::msg::String>("audio/heard", 10);
    RCLCPP_INFO(this->get_logger(), "Terminal input node started. Type text and press enter.");
    reader_thread_ = std::thread([this]() { this->stdin_loop(); });
  }

  ~TerminalInputNode() {
    running_ = false;
    if (reader_thread_.joinable()) reader_thread_.join();
  }

private:
  void stdin_loop() {
    std::string line;
    while (running_ && std::getline(std::cin, line)) {
      if (line.empty()) continue;
      auto msg = std_msgs::msg::String();
      msg.data = line;
      pub_->publish(msg);
      RCLCPP_INFO(this->get_logger(), "(terminal) published: '%s'", line.c_str());
    }
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
  std::thread reader_thread_;
  std::atomic<bool> running_{true};
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TerminalInputNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}