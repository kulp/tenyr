# For now we are configuring ctest into a subdirectory of the build directory
# that is chosen by the top-level Makefile. Eventually CMake will control the
# build directory location and this TENYR_LEGACY_BUILD_DIR will go away.
set(TENYR_LEGACY_BUILD_DIR "${CMAKE_BINARY_DIR}/..")

if (PLATFORM STREQUAL "mingw")
message("PLATFORM is MinGW")
# Configure wrappers for execution under Wine.

# Find the compiler
find_program(
    WRAPPED_CMAKE_TENYR_COMPILER
        NAMES "tas.exe"
        PATHS "${TENYR_LEGACY_BUILD_DIR}"
        DOC "tenyr assembler"
        NO_DEFAULT_PATH
)
mark_as_advanced(WRAPPED_CMAKE_TENYR_COMPILER)

set(WRAPPED_BIN ${WRAPPED_CMAKE_TENYR_COMPILER})
configure_file(${CMAKE_CURRENT_LIST_DIR}/wrap.sh.in ${CMAKE_BINARY_DIR}/tas @ONLY)
file(CHMOD ${CMAKE_BINARY_DIR}/tas FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)
set(CMAKE_TENYR_COMPILER ${CMAKE_BINARY_DIR}/tas)

find_program(
    WRAPPED_CMAKE_TENYR_LINKER
        NAMES "tld.exe"
        PATHS "${TENYR_LEGACY_BUILD_DIR}"
        DOC "tenyr linker"
        NO_DEFAULT_PATH
)
mark_as_advanced(WRAPPED_CMAKE_TENYR_LINKER)

set(WRAPPED_BIN ${WRAPPED_CMAKE_TENYR_LINKER})
configure_file(${CMAKE_CURRENT_LIST_DIR}/wrap.sh.in ${CMAKE_BINARY_DIR}/tld @ONLY)
file(CHMOD ${CMAKE_BINARY_DIR}/tld FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)
set(CMAKE_TENYR_LINKER ${CMAKE_BINARY_DIR}/tld)

# Although the simulator really is not the job of the containing CMake file, we
# set up the wrapper for that here, too.
find_program(
    WRAPPED_CMAKE_TENYR_SIMULATOR
        NAMES "tsim.exe"
        PATHS "${TENYR_LEGACY_BUILD_DIR}"
        DOC "tenyr simulator"
        NO_DEFAULT_PATH
)
mark_as_advanced(WRAPPED_CMAKE_TENYR_SIMULATOR)

set(WRAPPED_BIN ${WRAPPED_CMAKE_TENYR_SIMULATOR})
configure_file(${CMAKE_CURRENT_LIST_DIR}/wrap.sh.in ${CMAKE_BINARY_DIR}/tsim @ONLY)
file(CHMOD ${CMAKE_BINARY_DIR}/tsim FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)
# The TENYR_SIMULATOR variable is intentionally not named with a CMAKE_ prefix
# because it is not meant for use by CMake itself.
set(TENYR_SIMULATOR ${CMAKE_BINARY_DIR}/tsim)
else()
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
endif()

set(CMAKE_TENYR_SOURCE_FILE_EXTENSIONS tas)
set(CMAKE_TENYR_OUTPUT_EXTENSION .to)
set(CMAKE_TENYR_COMPILER_ENV_VAR "TENYR_AS")

# Configure variables set in this file for fast reload later on
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeTENYRCompiler.cmake.in
               ${CMAKE_PLATFORM_INFO_DIR}/CMakeTENYRCompiler.cmake)
