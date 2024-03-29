# TODO support non-Bash shells (including eventually cmd.exe on Windows).
set(CAPTURE_OUTPUT "${CMAKE_SOURCE_DIR}/cmake/capture_output.sh")

function(check_std_outputs)
    set(oneValueArgs NAME COMMAND INPUT)
    set(multiValueArgs ARGS PROPERTIES)
    cmake_parse_arguments(COS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    string(MAKE_C_IDENTIFIER "${COS_NAME}" COS_NAME)
    get_filename_component(COS_STEM "${COS_COMMAND}" NAME_WLE)

    if (NOT COS_INPUT)
        set(COS_INPUT "${CMAKE_SOURCE_DIR}/test/compare/${COS_STEM}/in/${COS_NAME}")
    else()
        # If a COS_INPUT file is specified, then inform CMake that we need a
        # fixture that supplies it.
        set(FIXTURES_REQUIRED "${COS_INPUT}")
    endif()

    if (NOT ICARUS AND COS_STEM STREQUAL "vvp")
        message("Skipping test generation for ${COS_NAME} because ICARUS is not enabled")
    else()
        add_test(
            NAME    "${COS_STEM}_${COS_NAME}_create"
            COMMAND ${CAPTURE_OUTPUT} "${COS_NAME}" ${COS_COMMAND} ${COS_ARGS} "${COS_INPUT}"
        )

        set_tests_properties(
            "${COS_STEM}_${COS_NAME}_create"
            PROPERTIES
                FIXTURES_SETUP "${COS_STEM}_${COS_NAME}"
                FIXTURES_REQUIRED "${FIXTURES_REQUIRED}"
                LABELS "tool=${COS_STEM}"
                ${COS_PROPERTIES}
        )

        add_test(
            NAME    "${COS_STEM}_${COS_NAME}_compare_stdout"
            COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol
                    "${CMAKE_SOURCE_DIR}/test/compare/${COS_STEM}/out/${COS_NAME}"
                    "${COS_NAME}.out"
        )

        set_tests_properties(
            "${COS_STEM}_${COS_NAME}_compare_stdout"
            PROPERTIES FIXTURES_REQUIRED "${COS_STEM}_${COS_NAME}"
            LABELS "tool=${COS_STEM}"
        )

        add_test(
            NAME    "${COS_STEM}_${COS_NAME}_compare_stderr"
            COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol
                    "${CMAKE_SOURCE_DIR}/test/compare/${COS_STEM}/err/${COS_NAME}"
                    "${COS_NAME}.err"
        )

        set_tests_properties(
            "${COS_STEM}_${COS_NAME}_compare_stderr"
            PROPERTIES FIXTURES_REQUIRED "${COS_STEM}_${COS_NAME}"
            LABELS "tool=${COS_STEM}"
        )
    endif()

endfunction()

