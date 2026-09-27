#include <rclcpp/rclcpp.hpp>
#include "memory_service/srv/add_memory.hpp"
#include "memory_service/srv/get_memory.hpp"
#include "memory_service/srv/list_memories.hpp"
#include "memory_service/srv/forget_person.hpp"
#include "memory_store.hpp"
#include <exception>
#include <functional>
#include <memory>
#include <string>

using Add = memory_service::srv::AddMemory;
using Get = memory_service::srv::GetMemory;
using List = memory_service::srv::ListMemories;
using Forget = memory_service::srv::ForgetPerson;

class MemoryServiceNode : public rclcpp::Node {
public:
  MemoryServiceNode() : Node("memory_service") {
    const auto path = declare_parameter<std::string>("storage_path", "memory_service.json");
    store_ = std::make_unique<memory_service::MemoryStore>(path);
    using std::placeholders::_1;
    using std::placeholders::_2;
    add_srv_ = create_service<Add>("memory/add", std::bind(&MemoryServiceNode::handle_add, this, _1, _2));
    get_srv_ = create_service<Get>("memory/get", std::bind(&MemoryServiceNode::handle_get, this, _1, _2));
    list_srv_ = create_service<List>("memory/list", std::bind(&MemoryServiceNode::handle_list, this, _1, _2));
    forget_srv_ = create_service<Forget>("memory/forget", std::bind(&MemoryServiceNode::handle_forget, this, _1, _2));
    RCLCPP_INFO(get_logger(), "Memory service ready");
  }

private:
  void handle_add(const std::shared_ptr<Add::Request> req, std::shared_ptr<Add::Response> res) {
    try {
      res->success = store_->add(req->person_id, req->key, req->value, req->consent);
      res->message = res->success ? "Stored" : "Consent required or invalid/limit exceeded";
    } catch (const std::exception &e) {
      RCLCPP_ERROR(get_logger(), "Memory add failed: %s", e.what());
      res->success = false;
      res->message = "Storage failure";
    }
  }
  void handle_get(const std::shared_ptr<Get::Request> req, std::shared_ptr<Get::Response> res) {
    const auto value = store_->get(req->person_id, req->key, req->consent);
    res->found = value.has_value();
    res->value = value.value_or("");
    res->message = !req->consent ? "Consent required" : res->found ? "OK" : "Not found";
  }
  void handle_list(const std::shared_ptr<List::Request> req, std::shared_ptr<List::Response> res) {
    res->success = req->consent && !req->person_id.empty() && req->person_id.size() <= 128;
    if (!res->success) {
      res->message = "Consent required or invalid person_id";
      return;
    }
    const auto rows = store_->list(req->person_id, req->limit, req->consent);
    for (const auto &[key, value] : rows) {
      res->keys.push_back(key);
      res->values.push_back(value);
    }
    res->message = "OK";
  }
  void handle_forget(const std::shared_ptr<Forget::Request> req, std::shared_ptr<Forget::Response> res) {
    if (req->person_id.empty() || req->person_id.size() > 128) {
      res->success = false;
      res->message = "Invalid person_id";
      return;
    }
    try {
      const bool removed = store_->forget(req->person_id);
      res->success = true; // Idempotent deletion; caller does not need consent to revoke it.
      res->message = removed ? "Forgotten" : "Already forgotten";
    } catch (const std::exception &e) {
      RCLCPP_ERROR(get_logger(), "Memory forget failed: %s", e.what());
      res->success = false;
      res->message = "Storage failure";
    }
  }

  std::unique_ptr<memory_service::MemoryStore> store_;
  rclcpp::Service<Add>::SharedPtr add_srv_;
  rclcpp::Service<Get>::SharedPtr get_srv_;
  rclcpp::Service<List>::SharedPtr list_srv_;
  rclcpp::Service<Forget>::SharedPtr forget_srv_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  try {
    auto node = std::make_shared<MemoryServiceNode>();
    rclcpp::spin(node);
  } catch (const std::exception &e) {
    RCLCPP_FATAL(rclcpp::get_logger("memory_service"), "Cannot start memory service: %s", e.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
