cmake_minimum_required(VERSION 2.8)
enable_testing()

project(common)
set(THIS_VERSION 1_0)
include("${CMAKE_SOURCE_DIR}/../cmake/common.cmake")

message(${PROJECT_NAME})

add_subdirectory(${PROJECT_NAME})
add_subdirectory(${PROJECT_NAME}_test)
add_subdirectory(external)


