# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.19...3.20)
cmake_minimum_required(VERSION 3.19)

project(tenyr VERSION 0.9.9)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(BUILD_EXAMPLES OFF "build examples from ex/")

# The TENYR language is self-hosted: tas, tld, and tsim are built by this
# project into ${CMAKE_BINARY_DIR}/src. CMake enables the TENYR language
# (which find_programs for tas) before add_subdirectory(src) builds it, so a
# clean configure with BUILD_EXAMPLES or TESTING finds no tas. Bootstrap the
# tools into build/src on demand (skipped once present) so a single
# `cmake -S . -B build -DBUILD_EXAMPLES=ON` or `-DTESTING=ON` works from
# scratch.
if(BUILD_EXAMPLES OR TESTING)
    set(_tenyr_tools_dir "${CMAKE_BINARY_DIR}/src")
    if(NOT EXISTS "${_tenyr_tools_dir}/tas")
        message(STATUS "tenyr: bootstrapping tas/tld/tsim into ${_tenyr_tools_dir}")
        set(_boot_dir "${CMAKE_BINARY_DIR}/_bootstrap")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -S "${CMAKE_SOURCE_DIR}" -B "${_boot_dir}"
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                    -DJIT=${JIT} -DSDL=${SDL} -DICARUS=${ICARUS}
                    -DBUILD_EXAMPLES=OFF -DTESTING=OFF
            RESULT_VARIABLE _boot_cfg_res)
        if(NOT _boot_cfg_res EQUAL 0)
            message(FATAL_ERROR "tenyr: bootstrap configure of tas/tld/tsim failed")
        endif()
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build "${_boot_dir}" --target tas tld tsim
            RESULT_VARIABLE _boot_bld_res)
        if(NOT _boot_bld_res EQUAL 0)
            message(FATAL_ERROR "tenyr: bootstrap build of tas/tld/tsim failed")
        endif()
        foreach(_t tas tld tsim)
            file(COPY "${_boot_dir}/src/${_t}" DESTINATION "${_tenyr_tools_dir}")
        endforeach()
        file(REMOVE_RECURSE "${_boot_dir}")
    endif()
endif()

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

add_subdirectory(lib)
add_subdirectory(hw/icarus)
add_subdirectory(hw/vpi)
add_subdirectory(src)
