cmake_minimum_required(VERSION 3.8)
project(gpt_bridge)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(memory_service REQUIRED)
find_package(CURL REQUIRED)

add_library(conversation_core src/conversation_core.cpp)
target_compile_features(conversation_core PUBLIC cxx_std_17)
target_include_directories(conversation_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(gpt_bridge_node src/gpt_bridge.cpp)
ament_target_dependencies(gpt_bridge_node rclcpp std_msgs memory_service)
target_link_libraries(gpt_bridge_node conversation_core CURL::libcurl)

if(BUILD_TESTING)
  enable_testing()
  add_executable(conversation_core_test tests/conversation_core_test.cpp)
  target_link_libraries(conversation_core_test conversation_core)
  # Assertion-based tests must also execute in Release builds.
  if(MSVC)
    target_compile_options(conversation_core_test PRIVATE /UNDEBUG)
  else()
    target_compile_options(conversation_core_test PRIVATE -UNDEBUG)
  endif()
  add_test(NAME conversation_core_test COMMAND conversation_core_test)
endif()

install(TARGETS gpt_bridge_node DESTINATION lib/${PROJECT_NAME})

ament_package()
