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
        WILL_FAIL TRUE
        ${CO_PROPERTIES}
    )
endfunction()

