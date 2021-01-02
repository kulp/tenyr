# For now, this file governs testing only, not building.

project(tenyr)

cmake_minimum_required(VERSION 3.12)

# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.12...3.19)

enable_testing()

function(check_failure)
    set(oneValueArgs NAME COMMAND EXPECT)
    set(multiValueArgs ARGUMENTS PROPERTIES)
    cmake_parse_arguments(CO "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    add_test(
        NAME "${CO_COMMAND}_${CO_NAME}_output"
        COMMAND ${CO_COMMAND} ${CO_ARGUMENTS}
    )
    set_tests_properties(
        "${CO_COMMAND}_${CO_NAME}_output"
        PROPERTIES
        LABELS "tool=${CO_COMMAND}"
        PASS_REGULAR_EXPRESSION "${CO_EXPECT}"
        ${CO_PROPERTIES}
    )

    add_test(
        NAME "${CO_COMMAND}_${CO_NAME}_exitcode"
        COMMAND ${CO_COMMAND} ${CO_ARGUMENTS}
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
    NAME        "invalid output file"
    EXPECT      "Failed to open"
    COMMAND     tas
    ARGUMENTS   -o . /dev/null
)

check_failure(
    NAME        "unhandled version"
    EXPECT      "Unhandled version"
    COMMAND     tas
    ARGUMENTS   -d ${OBJDIR}/bad_version.to
)

check_failure(
    NAME        "file size too large"
    EXPECT      "too large"
    COMMAND     tas
    ARGUMENTS   -d ${OBJDIR}/toolarge.to
)

check_failure(
    NAME        "too many symbols"
    EXPECT      "too large"
    COMMAND     tas
    ARGUMENTS   -d ${OBJDIR}/too-many-symbols.to
)

check_failure(
    NAME        "too many relocs"
    EXPECT      "too large"
    COMMAND     tas
    ARGUMENTS   -d ${OBJDIR}/too-many-relocs.to
)

check_failure(
    NAME        "overlong symbol"
    EXPECT      "too large"
    COMMAND     tas
    ARGUMENTS   -d ${OBJDIR}/overlong-symbol.to
)

check_failure(
    NAME        "overlong reloc"
    EXPECT      "too large"
    COMMAND     tas
    ARGUMENTS   -d ${OBJDIR}/overlong-reloc.to
)

check_failure(
    NAME        "missing global"
    EXPECT      "not defined"
    COMMAND     tas
    ARGUMENTS   ${TASDIR}/missing_global.tas
)

check_failure(
    NAME        "capture error message"
    EXPECT      "@q - @r"
    COMMAND     tas
    ARGUMENTS   ${CMAKE_SOURCE_DIR}/test/fail_compile/error_capture.tas
)

check_failure(
    NAME        "no backward memh support"
    EXPECT      "backward.*unsupported"
    COMMAND     tas
    ARGUMENTS   -d -f memh ${MEMHDIR}/backward.memh
)

check_failure(
    NAME        "bad format"
    EXPECT      "Usage:"
    COMMAND     tas
    ARGUMENTS   -f does_not_exist /dev/null
)

check_failure(
    NAME        "early end of file"
    EXPECT      "End of file unexpectedly reached"
    COMMAND     tld
    ARGUMENTS   /dev/null
)

check_failure(
    NAME        "invalid output file"
    EXPECT      "Failed to open"
    COMMAND     tld
    ARGUMENTS   -o . /dev/null
)

check_failure(
    NAME        "duplicate symbols"
    EXPECT      "Duplicate definition"
    COMMAND     tld
    ARGUMENTS   ${OBJDIR}/duplicate.to ${OBJDIR}/duplicate.to
)

check_failure(
    NAME        "multiple records"
    EXPECT      "more than one record"
    COMMAND     tld
    ARGUMENTS   ${OBJDIR}/tworecs.to
)

check_failure(
    NAME        "too many records"
    EXPECT      "too large"
    COMMAND     tld
    ARGUMENTS   ${OBJDIR}/too-many-records.to
)

check_failure(
    NAME        "unresolved"
    EXPECT      "Missing definition"
    COMMAND     tld
    ARGUMENTS   ${OBJDIR}/unresolved.to
)

check_failure(
    NAME        "negative relocation"
    EXPECT      "Invalid relocation"
    COMMAND     tld
    ARGUMENTS   ${OBJDIR}/invalid-reloc.to
)

check_failure(
    NAME        "too-large relocation"
    EXPECT      "Invalid relocation"
    COMMAND     tld
    ARGUMENTS   ${OBJDIR}/invalid-reloc2.to
)

check_failure(
    NAME        "bad format"
    EXPECT      "Usage:"
    COMMAND     tsim
    ARGUMENTS   -f does_not_exist /dev/null
)

check_failure(
    NAME        "bad recipe"
    EXPECT      "Usage:"
    COMMAND     tsim
    ARGUMENTS   -r does_not_exist /dev/null
)

foreach(tool ${TOOL_LIST})

    check_failure(
        NAME        "no args"
        EXPECT      "No input files specified"
        COMMAND     ${tool}
    )

    check_failure(
        NAME        "non-existent file"
        EXPECT      "Failed to open"
        COMMAND     ${tool}
        ARGUMENTS   does_not_exit
    )

    check_failure(
        NAME        "bad option"
        EXPECT      "invalid option.*Q"
        COMMAND     ${tool}
        ARGUMENTS   -QRSTU
    )

endforeach()
