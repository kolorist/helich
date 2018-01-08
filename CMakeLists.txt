cmake_minimum_required (VERSION 3.0)

project (helich)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/include")

file(GLOB_RECURSE file_list
    "${PROJECT_SOURCE_DIR}/src/cu/*.cpp")

add_definitions (-D_CRT_SECURE_NO_WARNINGS
	-DPLATFORM_WINDOWS)

add_library(helich ${file_list})

target_link_libraries (helich 
	floral)

target_include_directories (helich PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
