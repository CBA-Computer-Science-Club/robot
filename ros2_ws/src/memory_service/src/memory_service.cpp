#include <rclcpp/rclcpp.hpp>
#include "memory_service/srv/add_memory.hpp"
#include "memory_service/srv/get_memory.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>

using json = nlohmann::json;
using Add = memory_service::srv::AddMemory;
using Get = memory_service::srv::GetMemory;

class MemoryServiceNode : public rclcpp::Node {
public:
  MemoryServiceNode() : Node("memory_service") {
    add_srv_ = this->create_service<Add>("memory/add", std::bind(&MemoryServiceNode::handle_add, this, std::placeholders::_1, std::placeholders::_2));
    get_srv_ = this->create_service<Get>("memory/get", std::bind(&MemoryServiceNode::handle_get, this, std::placeholders::_1, std::placeholders::_2));
    storage_path_ = this->declare_parameter<std::string>("storage_path", this->get_node_options().get_node_name() + "_memory.json");
    load_storage();
    RCLCPP_INFO(this->get_logger(), "Memory service ready.");
  }

private:
  void load_storage() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ifstream ifs(storage_path_);
    if (ifs) {
      try { ifs >> store_; } catch(...) { store_ = json::object(); }
    } else {
      store_ = json::object();
    }
  }

  void save_storage() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ofstream ofs(storage_path_);
    ofs << store_.dump(2);
  }

  void handle_add(const std::shared_ptr<Add::Request> req, std::shared_ptr<Add::Response> res) {
    std::lock_guard<std::mutex> lock(mtx_);
    store_[req->key] = req->value;
    save_storage();
    res->success = true;
    res->message = "Stored";
  }

  void handle_get(const std::shared_ptr<Get::Request> req, std::shared_ptr<Get::Response> res) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (store_.contains(req->key)) {
      res->found = true;
      res->value = store_[req->key].get<std::string>();
      res->message = "OK";
    } else {
      res->found = false;
      res->value = "";
      res->message = "Not found";
    }
  }

  rclcpp::Service<Add>::SharedPtr add_srv_;
  rclcpp::Service<Get>::SharedPtr get_srv_;
  json store_;
  std::string storage_path_;
  std::mutex mtx_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<MemoryServiceNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}