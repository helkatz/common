#cmake_minimum_required(VERSION 3.0)

project(common)
string(TOUPPER ${PROJECT_NAME} UPPER_PROJECT_NAME)
set(MYLIB_VERSION "1_0")

message("   - ${PROJECT_NAME}")

file(GLOB_RECURSE sources src/* include/*)

add_library(${PROJECT_NAME} STATIC ${sources})

add_definitions("-D${UPPER_PROJECT_NAME}_STATIC=1")
if(BUILD_SHARED_LIBS)
	add_definitions("-D${UPPER_PROJECT_NAME}_STATIC=0")
endif()
#include (GenerateExportHeader)  
#GENERATE_EXPORT_HEADER(${PROJECT_NAME}           # generates the export header shared_EXPORTS.h automatically
#    BASE_NAME ${UPPER_PROJECT_NAME}
#    EXPORT_MACRO_NAME ${UPPER_PROJECT_NAME}_EXPORTS
#)
SOURCE_GROUP_BY_FOLDER(sources)


target_link_libraries(${PROJECT_NAME} PocoFoundation boost logger)
target_include_directories(${PROJECT_NAME} PUBLIC include)
#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/exports)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER helkatz)

set_target_properties(${PROJECT_NAME} PROPERTIES 
	OUTPUT_NAME ${PROJECT_NAME}
	DEBUG_OUTPUT_NAME ${PROJECT_NAME}d)	

CreateDist(${PROJECT_NAME} ${MYLIB_VERSION})
#add_custom_target(copy-runtime-files ALL
#    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/include ${DIST_DIR}/common
#    DEPENDS ${PROJECT_NAME})	