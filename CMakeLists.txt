# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.19...3.20)
cmake_minimum_required(VERSION 3.19)

project(tenyr VERSION 0.9.9 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(TESTING)
enable_language(TENYR)
enable_testing()

include(cmake/test_functions.cmake)
include("tests.cmake")

add_subdirectory(ex)
endif()

configure_file(src/tenyr_config.h.in tenyr_config.h)

add_subdirectory(lib)
add_subdirectory(hw/vpi)
add_subdirectory(src)
