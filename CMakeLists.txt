# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.19...3.20)
cmake_minimum_required(VERSION 3.19)

project(tenyr VERSION 0.9.9)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(BUILD_EXAMPLES OFF "build examples from ex/")

if(TESTING)
enable_language(TENYR)
enable_testing()

include(cmake/test_functions.cmake)
include("tests.cmake")

set(BUILD_EXAMPLES ON)
endif()

if(BUILD_EXAMPLES)
enable_language(TENYR)
add_subdirectory(ex)
endif()

include(ProcessorCount)
ProcessorCount(NPROC)

if(COVERAGE)
include(CodeCoverage)
append_coverage_compiler_flags()
setup_target_for_coverage_lcov(
    NAME            coverage
    EXECUTABLE      ctest -j ${NPROC}
    EXCLUDE         lexer.c parser.c
    NO_DEMANGLE
)
endif()

configure_file(src/tenyr_config.h.in tenyr_config.h)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # Emscripten builds need GNU extensions for EM_ASM and the FS API used in
    # the emscripten OS support layer.
    set(CMAKE_C_EXTENSIONS ON)

    # emscripten's headers define stdin/stdout/stderr as macros; suppress the
    # resulting -Wdisabled-macro-expansion warnings.
    add_compile_options(-Wno-disabled-macro-expansion)

    # Don't actually fclose() streams (e.g. stdin); fflush instead so that
    # main() can be re-invoked within a single emscripten module.
    add_compile_definitions(fclose=fflush)

    # pre.js configures stdout/stdin/locateFile for Node.js at link time.
    set(EM_PRE_JS ${CMAKE_SOURCE_DIR}/src/os/Emscripten/pre.js)
endif()

add_subdirectory(lib)
add_subdirectory(hw/icarus)
add_subdirectory(hw/vpi)
add_subdirectory(src)
