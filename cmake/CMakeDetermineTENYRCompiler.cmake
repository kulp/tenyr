# For now we are configuring ctest into a subdirectory of the build directory
# that is chosen by the top-level Makefile. Eventually CMake will control the
# build directory location and this TENYR_LEGACY_BUILD_DIR will go away.
set(TENYR_LEGACY_BUILD_DIR "${CMAKE_BINARY_DIR}/..")

# Find the compiler
find_program(
    CMAKE_TENYR_COMPILER
        NAMES "tas"
        PATHS "${TENYR_LEGACY_BUILD_DIR}"
        DOC "tenyr assembler"
        NO_DEFAULT_PATH
)
mark_as_advanced(CMAKE_TENYR_COMPILER)

find_program(
    CMAKE_TENYR_LINKER
        NAMES "tld"
        PATHS "${TENYR_LEGACY_BUILD_DIR}"
        DOC "tenyr linker"
        NO_DEFAULT_PATH
)
mark_as_advanced(CMAKE_TENYR_LINKER)

find_program(
    TENYR_SIMULATOR
        NAMES "tsim"
        PATHS "${TENYR_LEGACY_BUILD_DIR}"
        DOC "tenyr simulator"
        NO_DEFAULT_PATH
)
mark_as_advanced(TENYR_SIMULATOR)

set(CMAKE_TENYR_SOURCE_FILE_EXTENSIONS tas)
set(CMAKE_TENYR_OUTPUT_EXTENSION .to)
set(CMAKE_TENYR_COMPILER_ENV_VAR "TENYR_AS")

# Configure variables set in this file for fast reload later on
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeTENYRCompiler.cmake.in
               ${CMAKE_PLATFORM_INFO_DIR}/CMakeTENYRCompiler.cmake)
