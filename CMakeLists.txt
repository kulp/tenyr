# For now, this file governs testing only, not building.
# As a result, we initialize the project with no language support (C and CXX
# support would otherwise be enabled by default).

project(tenyr NONE)

cmake_minimum_required(VERSION 3.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
enable_language(TENYR)

# Define the versions of CMake that this file is written for.
cmake_policy(VERSION 3.12...3.19)

enable_testing()

include(cmake/test_functions.cmake)

set(TENYR_LEGACY_BUILD_DIR "${CMAKE_BINARY_DIR}/..")

add_subdirectory(lib)
add_subdirectory(ex)

function(check_success)
    set(oneValueArgs NAME COMMAND)
    set(multiValueArgs ARGS PROPERTIES)
    cmake_parse_arguments(CO "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    string(MAKE_C_IDENTIFIER "${CO_NAME}" CO_NAME)

    add_test(
        NAME "${CO_COMMAND}_${CO_NAME}_success"
        COMMAND ${CO_COMMAND} ${CO_ARGS}
    )
    set_tests_properties(
        "${CO_COMMAND}_${CO_NAME}_success"
        PROPERTIES
        LABELS "tool=${CO_COMMAND}"
        ${CO_PROPERTIES}
    )
endfunction()

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

file(GLOB fail_compile_INPUTS test/fail_compile/*.tas)
foreach(case ${fail_compile_INPUTS})

    get_filename_component(basename ${case} NAME_WE)
    check_failure(
        NAME    "fail_compile_${basename}"
        EXPECT  "syntax error|bailing|use before definition|Error|attempted"
        COMMAND tas
        ARGS    ${case}
    )

endforeach()

file(GLOB pass_compile_INPUTS test/pass_compile/*.tas)
foreach(case ${pass_compile_INPUTS})

    get_filename_component(basename ${case} NAME_WE)
    check_success(
        NAME    "pass_compile_${basename}"
        COMMAND tas
        ARGS    ${case}
    )

endforeach()

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

# CMake's regex syntax does not support {M,N} matching, so we have to expand it
# ourselves. This is a string of 67 dots; the longest acceptable line is 66
# characters.
set(TSIM_TOO_LONG "^...................................................................+$")
check_success(
    NAME    "narrow verbose output"
    COMMAND tsim
    ARGS    -vv ${OBJDIR}/empty.to
    PROPERTIES  FAIL_REGULAR_EXPRESSION ${TSIM_TOO_LONG}
)

check_failure(
    NAME    "missing @-file"
    EXPECT  "Error in opts file"
    COMMAND tsim
    ARGS    -@ does_not_exist -h
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

    check_output(
        NAME    "version option"
        EXPECT  "${tool} version.*built"
        COMMAND ${tool}
        ARGS    -V
    )

    check_output(
        NAME    "help option"
        EXPECT  "Usage: .*${tool}"
        COMMAND ${tool}
        ARGS    -h
    )

    check_output(
        NAME    "params option"
        EXPECT  "Usage: .*${tool}"
        COMMAND ${tool}
        ARGS    -p dummy=1 -h
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

check_std_outputs(NAME "text disassembly"       COMMAND tas     ARGS -ftext -d)
check_std_outputs(NAME "quiet disassembly"      COMMAND tas     ARGS -ftext -d -q)
check_std_outputs(NAME "verbose disassembly"    COMMAND tas     ARGS -ftext -d -v)
check_std_outputs(NAME "memh explicit"          COMMAND tas     ARGS -fmemh -pformat.memh.explicit=1)
check_std_outputs(NAME "memh offset"            COMMAND tas     ARGS -fmemh -pformat.memh.offset=5)
check_std_outputs(NAME "params overflow"        COMMAND tas     ARGS -fmemh -pA=1 -pB=1 -pC=1 -pD=1 -pE=1 -pF=1 -pG=1 -pH=1 -pI=1 -pJ=1 -pK=1 -pL=1 -pM=1 -pN=1 -pO=1 -pP=1 -pQ=1 -pR=1 -pS=1 -pT=1 -pU=1 -pV=1 -pW=1 -pX=1 -pY=1 -pZ=1 -pformat.memh.offset=5)

check_std_outputs(NAME "address invalid"        COMMAND tsim    ARGS -ftext -a 123 PROPERTIES WILL_FAIL TRUE)
check_std_outputs(NAME "address/start"          COMMAND tsim    ARGS -ftext -vvv -a 0x1011 -s 0x1012)
check_std_outputs(NAME "verbosity 1"            COMMAND tsim    ARGS -ftext -v)
check_std_outputs(NAME "verbosity 2"            COMMAND tsim    ARGS -ftext -vv)
check_std_outputs(NAME "verbosity 3"            COMMAND tsim    ARGS -ftext -vvv)
check_std_outputs(NAME "verbosity 4"            COMMAND tsim    ARGS -ftext -vvvv)
check_std_outputs(NAME "verbosity 5"            COMMAND tsim    ARGS -ftext -vvvvv)
check_std_outputs(NAME "multiple inputs fails"  COMMAND tsim    ARGS nonexistent_file_name another_inextant_file    PROPERTIES WILL_FAIL TRUE)

add_test(
    NAME    "tsim_stdin_accepted"
    COMMAND sh -c "echo | tsim -v -ftext -"
)
set_tests_properties(
    tsim_stdin_accepted
    PROPERTIES
        PASS_REGULAR_EXPRESSION "^IP = 0x00001000"
        LABELS "tool=tsim"
)

add_test(
    NAME "tsim_plugins_cap"
    COMMAND tsim -@ "${CMAKE_SOURCE_DIR}/test/misc/long.rcp" "${CMAKE_SOURCE_DIR}/test/misc/obj/empty.to"
)
set_tests_properties(
    "tsim_plugins_cap"
    PROPERTIES
        PASS_REGULAR_EXPRESSION "^Saw 17 plugins names, handling only the first 16"
        LABELS "tool=tsim"
)

add_test(
    NAME    "create deref.texe"
    COMMAND tas -o deref.texe ${CMAKE_SOURCE_DIR}/test/misc/deref.tas
)

set_tests_properties("create deref.texe" PROPERTIES FIXTURES_SETUP "deref.texe")

check_std_outputs(NAME "plugin no-error"        COMMAND tsim    ARGS -p "plugin[0]+=failure_NONE"           INPUT deref.texe)
check_std_outputs(NAME "plugin operation"       COMMAND tsim    ARGS -p "plugin[0]+=failure_OP"             INPUT deref.texe    PROPERTIES WILL_FAIL TRUE)
check_std_outputs(NAME "plugin initialisation"  COMMAND tsim    ARGS -p "plugin[0]+=failure_INIT"           INPUT deref.texe    PROPERTIES WILL_FAIL TRUE)
check_std_outputs(NAME "plugin finalisation"    COMMAND tsim    ARGS -p "plugin[0]+=failure_FINI"           INPUT deref.texe    PROPERTIES WILL_FAIL TRUE)
check_std_outputs(NAME "plugin nested init"     COMMAND tsim    ARGS -p "plugin[0]+=failure_PLUGIN_INIT"    INPUT deref.texe    PROPERTIES WILL_FAIL TRUE)

add_test(
    NAME "tsim_plugin_missing_symbol"
    COMMAND tsim -p "plugin[0]+=failure_PLUGIN_NO_ADD_DEVICE" ${CMAKE_SOURCE_DIR}/test/misc/obj/empty.to
)
set_tests_properties(
    "tsim_plugin_missing_symbol"
    PROPERTIES
        PASS_REGULAR_EXPRESSION "^Failed to find symbol `failure_PLUGIN_NO_ADD_DEVICE_add_device`"
        LABELS "tool=tsim"
)

check_std_outputs(NAME "deref zero failure"     COMMAND tsim                        INPUT deref.texe    PROPERTIES WILL_FAIL TRUE)
check_std_outputs(NAME "deref zero success"     COMMAND tsim    ARGS -rzero_word    INPUT deref.texe    PROPERTIES WILL_FAIL FALSE)

file(GLOB OPS test/op/*.tas)
foreach(op_path ${OPS})
    get_filename_component(op "${op_path}" NAME_WLE)
    add_test(
        NAME    "create ${op}.texe"
        COMMAND tas -o "${op}.texe" "${op_path}"
    )
    set_tests_properties("create ${op}.texe" PROPERTIES FIXTURES_SETUP "${op}.texe")

    check_std_outputs(NAME "op ${op}"   COMMAND tsim    ARGS -p tsim.dump_end_state=1   INPUT "${op}.texe")

endforeach()

check_std_outputs(
    NAME    "icarus failure nonexistent"
    COMMAND vvp
    ARGS    -M "${TENYR_LEGACY_BUILD_DIR}" -m vpidevices -N "${CMAKE_SOURCE_DIR}/hw/icarus/tenyr" "+LOAD=nonexistent.texe"
    PROPERTIES
        WILL_FAIL TRUE
)

set(icarus_stems bad_magic toolarge)
foreach(stem ${icarus_stems})

    add_test(
        NAME    "copy ${stem}.texe"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/test/misc/obj/${stem}.texe" "${CMAKE_CURRENT_BINARY_DIR}/"
    )
    set_tests_properties("copy ${stem}.texe" PROPERTIES FIXTURES_SETUP "${stem}.texe")

    check_std_outputs(
        NAME    "icarus failure ${stem}"
        COMMAND vvp
        ARGS    -M "${TENYR_LEGACY_BUILD_DIR}" -m vpidevices -N "${CMAKE_SOURCE_DIR}/hw/icarus/tenyr" "+LOAD=${stem}.texe"
        INPUT   "${stem}.texe"
        PROPERTIES
            WILL_FAIL TRUE
    )

endforeach()

add_test(
    NAME    "tld_stdin_accepted"
    COMMAND sh -c "tld - < ${CMAKE_SOURCE_DIR}/test/misc/obj/empty.to"
)
set_tests_properties(
    tld_stdin_accepted
    PROPERTIES
        PASS_REGULAR_EXPRESSION "^TOV"
        LABELS "tool=tld"
)

add_test(
    NAME    "tsim_irc_ping"
    COMMAND sh -c "echo PING 123 | tsim ex/irc.texe"
)
set_tests_properties(
    tsim_irc_ping
    PROPERTIES
        PASS_REGULAR_EXPRESSION "PONG 123"
        DEPENDS "create irc.texe"
        LABELS "tool=tsim"
)
