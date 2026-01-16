cmake_minimum_required(VERSION 3.8)
project(motor_controller)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(motor_controller_node src/motor_controller.cpp)
ament_target_dependencies(motor_controller_node rclcpp geometry_msgs)

# link lgpio if needed
find_library(LGPIO_LIB lgpio)
if(LGPIO_LIB)
  target_link_libraries(motor_controller_node ${LGPIO_LIB})
endif()

install(TARGETS
  motor_controller_node
  DESTINATION lib/${PROJECT_NAME})

ament_package()