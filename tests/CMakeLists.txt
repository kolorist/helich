cmake_minimum_required(VERSION 3.20min)

# utils
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake")
include(utils)

project (helich_unit_tests)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/../include")
include_directories("${PROJECT_SOURCE_DIR}/../../floral/include")

add_subdirectory("${PROJECT_SOURCE_DIR}/.." "helich")
add_subdirectory("${PROJECT_SOURCE_DIR}/../../floral" "floral")

file(GLOB_RECURSE file_list
	"${PROJECT_SOURCE_DIR}/include/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(helich_unit_tests ${file_list})

construct_msvc_filters_by_dir_scheme("${file_list}")

target_link_libraries(helich_unit_tests helich)
target_link_libraries(helich_unit_tests floral)
