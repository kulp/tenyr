# For now, this file governs testing only, not building.
# As a result, we initialize the project with no language support (C and CXX
# support would otherwise be enabled by default).

project(tenyr NONE)

cmake_minimum_required(VERSION 3.12)

# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.12...3.19)

enable_testing()

function(check_failure)
    set(oneValueArgs NAME COMMAND EXPECT)
    set(multiValueArgs ARGS PROPERTIES)
    cmake_parse_arguments(CO "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    string(MAKE_C_IDENTIFIER "${CO_NAME}" CO_NAME)

    add_test(
        NAME "${CO_NAME}_output"
        COMMAND ${CO_COMMAND} ${CO_ARGS}
    )
    set_tests_properties(
        "${CO_NAME}_output"
        PROPERTIES
        LABELS "tool=${CO_COMMAND}"
        PASS_REGULAR_EXPRESSION "${CO_EXPECT}"
        ${CO_PROPERTIES}
    )

    add_test(
        NAME "${CO_NAME}_exitcode"
        COMMAND ${CO_COMMAND} ${CO_ARGS}
    )
    set_tests_properties(
        "${CO_NAME}_exitcode"
        PROPERTIES
        LABELS "tool=${CO_COMMAND}"
        WILL_FAIL TRUE
        ${CO_PROPERTIES}
    )
endfunction()

set(TASDIR ${CMAKE_SOURCE_DIR}/test/misc)
set(OBJDIR ${TASDIR}/obj)
set(MEMHDIR ${TASDIR}/memh)

check_failure(
    NAME    "invalid output file"
    EXPECT  "Failed to open"
    COMMAND tas
    ARGS    -o . /dev/null
)

check_failure(
    NAME    "unhandled version"
    EXPECT  "Unhandled version"
    COMMAND tas
    ARGS    -d ${OBJDIR}/bad_version.to
)

check_failure(
    NAME    "file size too large"
    EXPECT  "too large"
    COMMAND tas
    ARGS    -d ${OBJDIR}/toolarge.to
)

check_failure(
    NAME    "too many symbols"
    EXPECT  "too large"
    COMMAND tas
    ARGS    -d ${OBJDIR}/too-many-symbols.to
)

check_failure(
    NAME    "too many relocs"
    EXPECT  "too large"
    COMMAND tas
    ARGS    -d ${OBJDIR}/too-many-relocs.to
)

check_failure(
    NAME    "overlong symbol"
    EXPECT  "too large"
    COMMAND tas
    ARGS    -d ${OBJDIR}/overlong-symbol.to
)

check_failure(
    NAME    "overlong reloc"
    EXPECT  "too large"
    COMMAND tas
    ARGS    -d ${OBJDIR}/overlong-reloc.to
)

check_failure(
    NAME    "missing global"
    EXPECT  "not defined"
    COMMAND tas
    ARGS    ${TASDIR}/missing_global.tas
)

check_failure(
    NAME    "capture error message"
    EXPECT  "@q - @r"
    COMMAND tas
    ARGS    ${CMAKE_SOURCE_DIR}/test/fail_compile/error_capture.tas
)

check_failure(
    NAME    "no backward memh support"
    EXPECT  "backward.*unsupported"
    COMMAND tas
    ARGS    -d -f memh ${MEMHDIR}/backward.memh
)

