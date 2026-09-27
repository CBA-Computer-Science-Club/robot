#include "conversation_core.hpp"
#include "memory_service/srv/add_memory.hpp"
#include "memory_service/srv/get_memory.hpp"

#include <curl/curl.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::chrono_literals;
using Add = memory_service::srv::AddMemory;
using Get = memory_service::srv::GetMemory;

namespace {
constexpr std::size_t kMaxUtterance = 2048;
constexpr std::size_t kMaxFact = 180;
constexpr std::size_t kMaxStoredFacts = 20;
constexpr std::size_t kMaxContextFacts = 3;
constexpr std::size_t kMaxContextBytes = 900;
constexpr std::size_t kMaxReplyBytes = 2048;
constexpr std::size_t kMaxHttpBytes = 65536;

// No external hosts, URL credentials, query strings, redirects or proxy access.
bool is_local_url(const std::string & url) {
  const auto end = url.find('/', 7);
  const auto authority = url.substr(7, end == std::string::npos ? std::string::npos : end - 7);
  const auto host_end = authority.find(':');
  const auto host = authority.substr(0, host_end);
  if (url.rfind("http://", 0) != 0 || url.find_first_of("@?#") != std::string::npos ||
      end == std::string::npos || host_end == std::string::npos ||
      (host != "localhost" && host != "127.0.0.1")) return false;
  const auto port = authority.substr(host_end + 1);
  return !port.empty() && port.find_first_not_of("0123456789") == std::string::npos;
}

size_t receive(char * data, size_t size, size_t count, void * context) {
  auto * body = static_cast<std::string *>(context);
  if (count && size > kMaxHttpBytes / count) return 0;
  const auto length = size * count;
  if (length > kMaxHttpBytes - body->size()) return 0;
  body->append(data, length);
  return length;
}
}  // namespace

class GptBridge : public rclcpp::Node {
public:
  GptBridge() : Node("gpt_bridge") {
    endpoint_ = declare_parameter<std::string>("endpoint", "http://127.0.0.1:11434/v1/chat/completions");
    model_ = declare_parameter<std::string>("model", "llama3.2");
    http_timeout_ms_ = std::clamp(declare_parameter<int>("http_timeout_ms", 12000), 1000, 60000);
    service_timeout_ms_ = std::clamp(declare_parameter<int>("memory_timeout_ms", 800), 100, 5000);
    memory_consent_ = declare_parameter<bool>("memory_consent", false);
    consented_person_id_ = declare_parameter<std::string>("consented_person_id", "");
    identity_mode_ = declare_parameter<std::string>("identity_mode", "disabled");
    identity_ttl_ms_ = std::clamp(declare_parameter<int>("identity_ttl_ms", 15000), 1000, 60000);
    if (!is_local_url(endpoint_)) throw std::invalid_argument("endpoint must be loopback HTTP with explicit port");
    if (model_.empty() || model_.size() > 128) throw std::invalid_argument("invalid model");
    if (!consented_person_id_.empty() && gpt_bridge::parse_identity(consented_person_id_) != consented_person_id_)
      throw std::invalid_argument("invalid consented_person_id");
    if (identity_mode_ != "disabled" && identity_mode_ != "operator_bound" && identity_mode_ != "trusted_topic")
      throw std::invalid_argument("invalid identity_mode");

    say_ = create_publisher<std_msgs::msg::String>("robot/say", 10);
    get_ = create_client<Get>("memory/get");
    add_ = create_client<Add>("memory/add");
    heard_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    rclcpp::SubscriptionOptions heard_options;
    heard_options.callback_group = heard_group_;
    heard_ = create_subscription<std_msgs::msg::String>("audio/heard", 10,
      [this](std_msgs::msg::String::ConstSharedPtr msg) { on_heard(msg->data); }, heard_options);
    identity_ = create_subscription<std_msgs::msg::String>("people/identity", 10,
      [this](std_msgs::msg::String::ConstSharedPtr msg) {
        std::lock_guard<std::mutex> lock(identity_mutex_);
        // Topic content is only a hint. Authorization also requires operator configuration.
        latest_identity_ = gpt_bridge::parse_identity(msg->data);
        identity_seen_at_ = std::chrono::steady_clock::now();
      });
    RCLCPP_INFO(get_logger(), "Local conversation bridge ready (memory disabled unless explicitly authorized).");
  }

private:
  void say(const std::string & text) {
    std_msgs::msg::String message;
    message.data = text;
    say_->publish(message);
  }

  bool memory_authorized() {
    if (!memory_consent_ || consented_person_id_.empty()) return false;
    if (identity_mode_ == "operator_bound") return true;  // operator attests exclusive session
    if (identity_mode_ != "trusted_topic") return false;
    std::lock_guard<std::mutex> lock(identity_mutex_);
    return latest_identity_ == consented_person_id_ &&
           std::chrono::steady_clock::now() - identity_seen_at_ <=
             std::chrono::milliseconds(identity_ttl_ms_);
  }

  std::string memory_key() const { return "facts"; }

  // A missing key is normal; a transport timeout is not. Never log service response payloads.
  bool load_facts(std::string & value) {
    if (!get_->wait_for_service(std::chrono::milliseconds(service_timeout_ms_))) return false;
    auto req = std::make_shared<Get::Request>();
    req->person_id = consented_person_id_;
    req->key = memory_key();
    req->consent = true;
    auto pending = get_->async_send_request(req);
    if (pending.wait_for(std::chrono::milliseconds(service_timeout_ms_)) != std::future_status::ready) {
      get_->remove_pending_request(pending.request_id);
      return false;
    }
    try {
      const auto res = pending.get();
      value = res->found ? res->value : "";
      if (value.size() > 2048) return false;
      if (!value.empty() && !nlohmann::json::parse(value, nullptr, false).is_array()) return false;
      return true;
    } catch (const std::exception &) { return false; }
  }

  bool store_fact(const std::string & fact) {
    std::string old_value;
    if (!load_facts(old_value)) return false;
    std::string next;
    try { next = gpt_bridge::append_fact(old_value, fact, kMaxStoredFacts, kMaxFact); }
    catch (const std::exception &) { return false; }
    if (!add_->wait_for_service(std::chrono::milliseconds(service_timeout_ms_))) return false;
    auto req = std::make_shared<Add::Request>();
    req->person_id = consented_person_id_;
    req->key = memory_key();
    req->consent = true;
    req->value = next;
    auto pending = add_->async_send_request(req);
    if (pending.wait_for(std::chrono::milliseconds(service_timeout_ms_)) != std::future_status::ready) {
      add_->remove_pending_request(pending.request_id);
      return false;
    }
    try { if (!pending.get()->success) return false; }
    catch (const std::exception &) { return false; }
    // A positive write acknowledgement alone is insufficient: check the actual stored value.
    std::string verified;
    return load_facts(verified) && verified == next;
  }

  std::string local_completion(const std::string & body) {
    if (body.size() > 16384) throw std::runtime_error("request too large");
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
    if (!curl) throw std::runtime_error("HTTP initialization failed");
    curl_slist * raw_headers = curl_slist_append(nullptr, "Content-Type: application/json");
    if (!raw_headers) throw std::runtime_error("HTTP header allocation failed");
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw_headers, curl_slist_free_all);
    std::string response;
    curl_easy_setopt(curl.get(), CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, receive);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, 1500L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(http_timeout_ms_));
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt(curl.get(), CURLOPT_PROXY, "");
    const auto code = curl_easy_perform(curl.get());
    long http_status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_status);
    if (code != CURLE_OK || http_status != 200) throw std::runtime_error("local model HTTP failure");
    auto reply = gpt_bridge::parse_reply(response);
    if (reply.size() > kMaxReplyBytes) throw std::runtime_error("model reply too large");
    return reply;
  }

  void on_heard(const std::string & utterance) {
    if (utterance.empty()) return;
    if (utterance.size() > kMaxUtterance) {
      say("Sorry, that message was too long.");
      return;
    }
    // Explicit commands are NEVER sent to the model and are the only path to AddMemory.
    // Detect the prefix independently so an empty/oversized fact cannot fall through to the model.
    const auto command = utterance.find_first_not_of(" \t\r\n");
    if (command != std::string::npos && utterance.size() - command >= 9) {
      auto prefix = utterance.substr(command, 9);
      std::transform(prefix.begin(), prefix.end(), prefix.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      if (prefix == "remember:") {
        const auto fact = gpt_bridge::parse_remember(utterance, kMaxFact);
        if (!fact) { say("I couldn't save that. Say 'Remember:' followed by a short fact."); return; }
        if (!memory_authorized()) {
          say("I can't save memories without consent and a verified session identity."); return;
        }
        if (store_fact(*fact)) say("I remembered that.");
        else { RCLCPP_ERROR(get_logger(), "Memory write or verification failed.");
               say("I couldn't save that memory. Please try again later."); }
        return;
      }
    }
    std::vector<std::string> facts;
    if (memory_authorized()) {
      std::string stored;
      if (load_facts(stored)) facts = gpt_bridge::read_facts(stored, kMaxContextFacts, 300);
      else { RCLCPP_WARN(get_logger(), "Memory read timed out or unavailable.");
             say("Memory is unavailable; continuing without it."); }
    }
    try {
      say(local_completion(gpt_bridge::build_request(model_, utterance, facts,
                                                      kMaxContextFacts, kMaxContextBytes)));
    } catch (const std::exception &) {
      RCLCPP_ERROR(get_logger(), "Local model request failed.");
      say("Sorry, I can't reach my local language model right now.");
    }
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr say_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr heard_, identity_;
  rclcpp::CallbackGroup::SharedPtr heard_group_;
  rclcpp::Client<Get>::SharedPtr get_;
  rclcpp::Client<Add>::SharedPtr add_;
  std::mutex identity_mutex_;
  std::string latest_identity_;
  std::chrono::steady_clock::time_point identity_seen_at_{};
  std::string endpoint_, model_, consented_person_id_, identity_mode_;
  int http_timeout_ms_{}, service_timeout_ms_{}, identity_ttl_ms_{};
  bool memory_consent_{};
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  int result = 0;
  try {
    rclcpp::executors::MultiThreadedExecutor executor(rclcpp::ExecutorOptions(), 3);
    executor.add_node(std::make_shared<GptBridge>());
    executor.spin();
  } catch (const std::exception & error) {
    // Error text from parameters may contain secrets; keep diagnostics generic.
    (void)error;
    RCLCPP_ERROR(rclcpp::get_logger("gpt_bridge"), "Bridge configuration or executor failure.");
    result = 1;
  }
  curl_global_cleanup();
  rclcpp::shutdown();
  return result;
}
