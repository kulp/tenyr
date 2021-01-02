# For now, this file governs testing only, not building.
# As a result, we initialize the project with no language support (C and CXX
# support would otherwise be enabled by default).

project(tenyr NONE)

cmake_minimum_required(VERSION 3.12)

# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.12...3.19)

enable_testing()

function(check_output)
    set(oneValueArgs NAME COMMAND EXPECT)
    set(multiValueArgs ARGS PROPERTIES)
    cmake_parse_arguments(CO "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    string(MAKE_C_IDENTIFIER "${CO_NAME}" CO_NAME)

    add_test(
        NAME "${CO_COMMAND}_${CO_NAME}_output"
        COMMAND ${CO_COMMAND} ${CO_ARGS}
    )
    set_tests_properties(
        "${CO_COMMAND}_${CO_NAME}_output"
        PROPERTIES
        LABELS "tool=${CO_COMMAND}"
        PASS_REGULAR_EXPRESSION "${CO_EXPECT}"
        ${CO_PROPERTIES}
    )
endfunction()

function(check_failure)
    set(oneValueArgs NAME COMMAND EXPECT)
    set(multiValueArgs ARGS PROPERTIES)
    cmake_parse_arguments(CO "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    string(MAKE_C_IDENTIFIER "${CO_NAME}" CO_NAME)

    check_output(
        NAME    "${CO_NAME}"
        EXPECT  "${CO_EXPECT}"
        COMMAND "${CO_COMMAND}"
        ARGS    "${CO_ARGS}"
        PROPERTIES  "${CO_PROPERTIES}"
    )

    add_test(
        NAME "${CO_COMMAND}_${CO_NAME}_exitcode"
        COMMAND ${CO_COMMAND} ${CO_ARGS}
    )
    set_tests_properties(
        "${CO_COMMAND}_${CO_NAME}_exitcode"
        PROPERTIES
        LABELS "tool=${CO_COMMAND}"
        WILL_FAIL TRUE
        ${CO_PROPERTIES}
    )
endfunction()

set(TOOL_LIST tas tld tsim)

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

check_failure(
    NAME    "bad format"
    EXPECT  "Usage:"
    COMMAND tas
    ARGS    -f does_not_exist /dev/null
)

check_failure(
    NAME    "early end of file"
    EXPECT  "End of file unexpectedly reached"
    COMMAND tld
    ARGS    /dev/null
)

check_failure(
    NAME    "invalid output file"
    EXPECT  "Failed to open"
    COMMAND tld
    ARGS    -o . /dev/null
)

check_failure(
    NAME    "duplicate symbols"
    EXPECT  "Duplicate definition"
    COMMAND tld
    ARGS    ${OBJDIR}/duplicate.to ${OBJDIR}/duplicate.to
)

check_failure(
    NAME    "multiple records"
    EXPECT  "more than one record"
    COMMAND tld
    ARGS    ${OBJDIR}/tworecs.to
)

check_failure(
    NAME    "too many records"
    EXPECT  "too large"
    COMMAND tld
    ARGS    ${OBJDIR}/too-many-records.to
)

check_failure(
    NAME    "unresolved"
    EXPECT  "Missing definition"
    COMMAND tld
    ARGS    ${OBJDIR}/unresolved.to
)

check_failure(
    NAME    "negative relocation"
    EXPECT  "Invalid relocation"
    COMMAND tld
    ARGS    ${OBJDIR}/invalid-reloc.to
)

check_failure(
    NAME    "too-large relocation"
    EXPECT  "Invalid relocation"
    COMMAND tld
    ARGS    ${OBJDIR}/invalid-reloc2.to
)

check_failure(
    NAME    "bad format"
    EXPECT  "Usage:"
    COMMAND tsim
    ARGS    -f does_not_exist /dev/null
)

check_failure(
    NAME    "bad recipe"
    EXPECT  "Usage:"
    COMMAND tsim
    ARGS    -r does_not_exist /dev/null
)

foreach(tool ${TOOL_LIST})

    check_failure(
        NAME    "no args"
        EXPECT  "No input files specified"
        COMMAND ${tool}
    )

    check_failure(
        NAME    "non-existent file"
        EXPECT  "Failed to open"
        COMMAND ${tool}
        ARGS    does_not_exit
    )

    check_failure(
        NAME    "bad option"
        EXPECT  "option.*Q"
        COMMAND ${tool}
        ARGS    -QRSTU
    )

endforeach()

file(
    GLOB dogfood_INPUTS
    ${CMAKE_SOURCE_DIR}/test/pass_compile/*.tas
    ${CMAKE_SOURCE_DIR}/ex/*.tas
)

add_test(
    NAME dogfood
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/dogfood.sh dogfood.XXXXXX tas ${dogfood_INPUTS}
)
set_tests_properties(
    dogfood
    PROPERTIES
    LABELS "tool=tas;expensive"
    COST 0.5
    TIMEOUT 60
)

