cmake_minimum_required(VERSION 3.8)
project(memory_service)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(nlohmann_json REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/AddMemory.srv"
  "srv/GetMemory.srv"
  "srv/ListMemories.srv"
  "srv/ForgetPerson.srv"
)

add_executable(memory_service_node src/memory_service.cpp src/memory_store.cpp)
target_compile_features(memory_service_node PUBLIC cxx_std_17)
ament_target_dependencies(memory_service_node rclcpp)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(memory_service_node nlohmann_json::nlohmann_json "${cpp_typesupport_target}")

install(TARGETS memory_service_node DESTINATION lib/${PROJECT_NAME})
ament_package()
