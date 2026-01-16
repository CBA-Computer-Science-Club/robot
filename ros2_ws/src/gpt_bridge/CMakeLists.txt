cmake_minimum_required(VERSION 3.8)
project(gpt_bridge)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(gpt_bridge_node src/gpt_bridge.cpp)
ament_target_dependencies(gpt_bridge_node rclcpp std_msgs nlohmann_json)
target_link_libraries(gpt_bridge_node CURL::libcurl)

install(TARGETS
  gpt_bridge_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()