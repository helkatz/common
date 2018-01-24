#cmake_minimum_required(VERSION 3.0)
project(common_test)
message("   - ${PROJECT_NAME}")

file(GLOB_RECURSE sources src/* include/*)

add_executable(${PROJECT_NAME} ${sources})
SOURCE_GROUP_BY_FOLDER(sources)

target_link_libraries(${PROJECT_NAME} common gtest_main PocoFoundation PocoDataMySQL PocoDataSQLite)

target_include_directories(${PROJECT_NAME} PUBLIC include)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER helkatz)
