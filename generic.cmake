cmake_minimum_required(VERSION 2.8)

function(CreateDist target version)	
	set(distDir ${DIST_DIR}/${target}/${version})
	set(sourceDir ${CMAKE_SOURCE_DIR}/${target})
	set(targetDir ${PROJECT_BINARY_DIR}/${target}/${version})
	message("distDir=${distDir}")
	message("sourceDir=${sourceDir}")
	message("sourceDir=${sourceDir}")
	file(MAKE_DIRECTORY ${distDir}/include)
	file(MAKE_DIRECTORY ${distDir}/lib)
	file(MAKE_DIRECTORY ${distDir}/bin)
	
	# copy headers and libs to the distribution folder
	add_custom_command(TARGET ${target} POST_BUILD
		#COMMAND -E make_directory ${distDir}/include
		COMMAND cmake -E copy_directory ${sourceDir}/include ${distDir}/include
		COMMAND cmake -E copy $<TARGET_FILE:common> ${distDir}/lib
		)
	# create the cmake file to use the library
	file(WRITE ${distDir}/common.cmake 
		"add_library(common STATIC IMPORTED GLOBAL)\n"
		"set_target_properties(faye PROPERTIES\n"
		"	INTERFACE_INCLUDE_DIRECTORIES \${CMAKE_CURRENT_SOURCE_DIR}/include\n"
		"	IMPORTED_LOCATION \${CMAKE_CURRENT_SOURCE_DIR}/lib/common.lib\n"
		"	IMPORTED_LOCATION_DEBUG \${CMAKE_CURRENT_SOURCE_DIR}/lib/commond.lib\n"
		"	INTERFACE_COMPILE_DEFINITIONS \"COMMON_STATIC=1\")"
	)
endfunction()

MACRO(SOURCE_GROUP_BY_FOLDER source_files)
  SET(SOURCE_GROUP_DELIMITER "/")
  SET(last_dir "")
  SET(files "")
#  FOREACH(file ${${target}_SRC} ${${target}_HEADERS})
  FOREACH(file ${${source_files}})
    file(RELATIVE_PATH relative_file "${PROJECT_SOURCE_DIR}" ${file})
    GET_FILENAME_COMPONENT(dir "${relative_file}" PATH)
    IF (NOT "${dir}" STREQUAL "${last_dir}")
      IF (files)
        SOURCE_GROUP("${last_dir}" FILES ${files})
      ENDIF (files)
      SET(files "")
    ENDIF (NOT "${dir}" STREQUAL "${last_dir}")
    SET(files ${files} ${file})
    SET(last_dir "${dir}")
  ENDFOREACH(file)
  IF (files)
    SOURCE_GROUP("${last_dir}" FILES ${files})
  ENDIF (files)
ENDMACRO(SOURCE_GROUP_BY_FOLDER)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP /Zi /DEBUG")
	SET(CMAKE_LINKER_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG")
else()
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -Wall -std=c++1z -static-libstdc++ -Wno-error=unused -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=deprecated-declarations -Wno-deprecated")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -std=c++1z -static-libstdc++ -Wno-error=unused -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=deprecated-declarations -Wno-deprecated")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)



