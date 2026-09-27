#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <atomic>
#include <cerrno>
#include <poll.h>
#include <string>
#include <thread>
#include <unistd.h>

class TerminalInputNode : public rclcpp::Node {
public:
  TerminalInputNode() : Node("terminal_input") {
    pub_ = create_publisher<std_msgs::msg::String>("audio/heard", 10);
    RCLCPP_INFO(get_logger(), "Terminal text input ready (not a microphone). Type and press Enter.");
    reader_thread_ = std::thread([this]() { stdin_loop(); });
  }

  ~TerminalInputNode() override {
    running_ = false;
    if (reader_thread_.joinable()) reader_thread_.join();
  }

private:
  void stdin_loop() {
    std::string line;
    bool discarding = false;
    while (running_) {
      pollfd input{STDIN_FILENO, POLLIN, 0};
      const int ready = ::poll(&input, 1, 100);
      if (ready < 0) {
        if (errno == EINTR) continue;
        break;
      }
      if (ready == 0) continue;
      if (!(input.revents & POLLIN)) break;
      char ch{};
      if (::read(STDIN_FILENO, &ch, 1) <= 0) break;
      if (ch == '\n') {
        if (!discarding && !line.empty()) {
          if (line.back() == '\r') line.pop_back();
          if (!line.empty()) {
            std_msgs::msg::String message;
            message.data = line;
            pub_->publish(message);
          }
        }
        line.clear();
        discarding = false;
      } else if (!discarding) {
        if (line.size() < 2048) line.push_back(ch);
        else { line.clear(); discarding = true; }
      }
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
