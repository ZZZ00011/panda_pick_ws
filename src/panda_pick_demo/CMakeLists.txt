cmake_minimum_required(VERSION 3.16)
project(panda_pick_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)
find_package(generate_parameter_library REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(moveit_core REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(moveit_task_constructor_core REQUIRED)
find_package(moveit_task_constructor_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(shape_msgs REQUIRED)
find_package(tf2_eigen REQUIRED)

generate_parameter_library(pick_place_demo_parameters src/pick_place_demo_parameters.yaml)

add_executable(pick_place_demo src/pick_place_demo.cpp src/pick_place_task.cpp)
target_include_directories(pick_place_demo PRIVATE include)
target_link_libraries(pick_place_demo pick_place_demo_parameters)
ament_target_dependencies(pick_place_demo
  geometry_msgs
  moveit_core
  moveit_msgs
  moveit_task_constructor_core
  moveit_task_constructor_msgs
  rclcpp
  shape_msgs
  tf2_eigen
)

install(TARGETS pick_place_demo DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})
install(FILES LICENSE DESTINATION share/${PROJECT_NAME})

ament_package()
