#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <cstdlib>
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GptBridge : public rclcpp::Node {
public:
  GptBridge() : Node("gpt_bridge") {
    sub_ = this->create_subscription<std_msgs::msg::String>("audio/heard", 10, std::bind(&GptBridge::on_heard, this, std::placeholders::_1));
    OPENAI_KEY = std::getenv("OPENAI_API_KEY") ? std::getenv("OPENAI_API_KEY") : "";
    if (OPENAI_KEY.empty()) {
      RCLCPP_WARN(this->get_logger(), "OPENAI_API_KEY not set. gpt_bridge will not call OpenAI.");
    }
  }

private:
  void on_heard(const std_msgs::msg::String::SharedPtr msg) {
    RCLCPP_INFO(this->get_logger(), "Heard: '%s'", msg->data.c_str());
    if (OPENAI_KEY.empty()) return;
    // Minimal example: call OpenAI Chat Completions (POST), parse response.
    // TODO: implement robust HTTP request (curl) and tool-to-topic mapping.
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
  std::string OPENAI_KEY;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GptBridge>());
  rclcpp::shutdown();
  return 0;
}