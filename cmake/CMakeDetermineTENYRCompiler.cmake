# Find the compiler
find_program(
    CMAKE_TENYR_COMPILER
        NAMES "tas"
        HINTS "${CMAKE_SOURCE_DIR}"
        DOC "tenyr assembler"
)
mark_as_advanced(CMAKE_TENYR_COMPILER)

find_program(
    CMAKE_TENYR_LINKER
        NAMES "tld"
        HINTS "${CMAKE_SOURCE_DIR}"
        DOC "tenyr linker"
)
mark_as_advanced(CMAKE_TENYR_LINKER)

set(CMAKE_TENYR_SOURCE_FILE_EXTENSIONS tas)
set(CMAKE_TENYR_OUTPUT_EXTENSION .to)
set(CMAKE_TENYR_COMPILER_ENV_VAR "TENYR_AS")

# Configure variables set in this file for fast reload later on
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeTENYRCompiler.cmake.in
               ${CMAKE_PLATFORM_INFO_DIR}/CMakeTENYRCompiler.cmake)
