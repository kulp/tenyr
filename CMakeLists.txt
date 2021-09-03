# For now, this file governs testing only, not building.
# As a result, we initialize the project with no language support (C and CXX
# support would otherwise be enabled by default).

project(tenyr NONE)

cmake_minimum_required(VERSION 3.19)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
enable_language(TENYR)

# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.19...3.20)

enable_testing()

include(cmake/test_functions.cmake)

set(TENYR_LEGACY_BUILD_DIR "${CMAKE_BINARY_DIR}/..")

add_subdirectory(lib)
add_subdirectory(ex)

include("tests.cmake")
