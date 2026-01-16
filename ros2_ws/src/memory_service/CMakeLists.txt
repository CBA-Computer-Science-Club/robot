cmake_minimum_required(VERSION 3.8)
project(memory_service)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/AddMemory.srv"
  "srv/GetMemory.srv"
)

find_package(nlohmann_json REQUIRED)

add_executable(memory_service_node src/memory_service.cpp)
ament_target_dependencies(memory_service_node rclcpp nlohmann_json)

rosidl_target_interfaces(memory_service_node ${PROJECT_NAME} "rosidl_typesupport_cpp")

install(TARGETS memory_service_node DESTINATION lib/${PROJECT_NAME})
ament_package()