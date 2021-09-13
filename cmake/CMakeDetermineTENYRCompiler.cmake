# Find the compiler
find_program(
    CMAKE_TENYR_COMPILER
        NAMES "tas"
        DOC "tenyr assembler"
        PATHS "${CMAKE_BINARY_DIR}/src"
        NO_DEFAULT_PATH
        REQUIRED
)
mark_as_advanced(CMAKE_TENYR_COMPILER)

find_program(
    CMAKE_TENYR_LINKER
        NAMES "tld"
        DOC "tenyr linker"
        PATHS "${CMAKE_BINARY_DIR}/src"
        NO_DEFAULT_PATH
        REQUIRED
)
mark_as_advanced(CMAKE_TENYR_LINKER)

find_program(
    TENYR_SIMULATOR
        NAMES "tsim"
        DOC "tenyr simulator"
        PATHS "${CMAKE_BINARY_DIR}/src"
        NO_DEFAULT_PATH
        REQUIRED
)
mark_as_advanced(TENYR_SIMULATOR)

set(CMAKE_TENYR_SOURCE_FILE_EXTENSIONS tas)
set(CMAKE_TENYR_OUTPUT_EXTENSION .to)
set(CMAKE_TENYR_COMPILER_ENV_VAR "TENYR_AS")

# Configure variables set in this file for fast reload later on
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeTENYRCompiler.cmake.in
               ${CMAKE_PLATFORM_INFO_DIR}/CMakeTENYRCompiler.cmake)
