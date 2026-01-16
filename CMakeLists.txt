cmake_minimum_required(VERSION 3.8)
project(robot_ai_cpp)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)

# Link the GPIO library for Pi 5
find_library(LGPIO_LIB lgpio)

add_executable(ai_driver src/ai_driver.cpp)
target_include_directories(ai_driver PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  
)

add_executable(motor_controller_exe src/motor_controller.cpp)

# link everything together
target_link_libraries(ai_driver 
  ${LGPIO_LIB}
  rclcpp::rclcpp
  geometry_msgs::geometry_msgs
)

ament_package()