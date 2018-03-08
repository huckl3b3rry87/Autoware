cmake_minimum_required(VERSION 2.8.3)
project(mavs_gazebo)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  sensor_msgs
  tf
  global_planner
  base_local_planner
)

catkin_package(
  CATKIN_DEPENDS
  roscpp
  sensor_msgs
  tf
  global_planner
  base_local_planner
)
