cmake_minimum_required(VERSION 3.8)
project(terminal_input)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(terminal_input_node src/terminal_input.cpp)
ament_target_dependencies(terminal_input_node rclcpp std_msgs)

install(TARGETS
  terminal_input_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()