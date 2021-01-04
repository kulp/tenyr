set(examples
    bm_chars
    bm_conway
    bm_fib
    bm_gpio
    bm_mults
    bm_snake
    bsearch_demo
    clock
    compare
    copy
    fib
    fib_iter
    hello
    irc
    isprime
    maths
    parse_hex
    qsort_demo
    set
    sieve
    test_imul
    totient
    trailz_demo
)

add_custom_target(examples)

foreach(ex ${examples})

    add_executable(${ex}.texe ${ex}.tas)
    add_dependencies(examples ${ex}.texe)

endforeach()

target_sources(maths.texe PRIVATE dword_add.tas dword_mul.tas)
target_sources(bm_conway.texe PRIVATE bm_conway_buffer.tas)

target_link_libraries(hello.texe libtenyr_puts)
target_link_libraries(compare.texe libtenyr_strcmp libtenyr_puts)
target_link_libraries(maths.texe libtenyr_isqrt libtenyr_umod libtenyr_udiv)
target_link_libraries(totient.texe libtenyr_gcd)
target_link_libraries(isprime.texe libtenyr_umod libtenyr_isqrt libtenyr_udiv)
target_link_libraries(bsearch_demo.texe libtenyr_bsearch libtenyr_puts)
target_link_libraries(qsort_demo.texe libtenyr_qsort libtenyr_puts libtenyr_memcpy)
target_link_libraries(irc.texe libtenyr_puts libtenyr_strcmp libtenyr_udiv libtenyr_umod libtenyr_strtol)
target_link_libraries(parse_hex.texe libtenyr_strtol)
target_link_libraries(bm_snake.texe libtenyr_rand)
target_link_libraries(bm_conway.texe libtenyr_rand)
target_link_libraries(bm_fib.texe libtenyr_putnum)
target_link_libraries(bm_mults.texe libtenyr_putnum)
target_link_libraries(clock.texe libtenyr_sleep)
target_link_libraries(trailz_demo.texe libtenyr_puts libtenyr_trailz)
target_link_libraries(test_imul.texe libtenyr_imul libtenyr_puts libtenyr_print_hex)
target_link_libraries(copy.texe libtenyr_memcpy libtenyr_puts)

set(demos bsearch qsort trailz)

foreach(demo ${demos})

    set(tsim_flags "")
    set(reference_file ${CMAKE_SOURCE_DIR}/cmake/golden/${demo}_out.txt)
    set(demo_executable ${CMAKE_CURRENT_BINARY_DIR}/${demo}_demo.texe)

    set(output_file ${CMAKE_CURRENT_BINARY_DIR}/tsim_${demo}_out.txt)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/run_tsim.in
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/run_tsim_${demo}.cmd
        @ONLY
    )

    file(
        COPY        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/run_tsim_${demo}.cmd
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )

    add_test(
        NAME        run_tsim_${demo}_demo_run
        COMMAND     run_tsim_${demo}.cmd
    )

    add_test(
        NAME        run_tsim_${demo}_demo_compare
        COMMAND     ${CMAKE_COMMAND} -E compare_files "${reference_file}" "${output_file}"
    )
    set_tests_properties(
        run_tsim_${demo}_demo_compare
        PROPERTIES DEPENDS run_tsim_${demo}_demo_run
    )

    set(logfile ${CMAKE_CURRENT_BINARY_DIR}/logfile_${demo})
    set(output_file ${CMAKE_CURRENT_BINARY_DIR}/icarus_${demo}_out.txt)
    # Because `vvp` emits warnings and errors to stdout even when `-l` is used,
    # we capture a separate logfile, concatenate it with the original reference
    # file, and compare with that.
    set(reference_temporary ${CMAKE_CURRENT_BINARY_DIR}/icarus_${demo}_out2.txt)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/run_icarus.in
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/run_icarus_${demo}.cmd
        @ONLY
    )

    file(
        COPY        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/run_icarus_${demo}.cmd
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )

    add_test(
        NAME        run_icarus_${demo}_demo_run
        COMMAND     run_icarus_${demo}.cmd
        PROPERTIES  COST 0.5
    )

    set(reference_file ${reference_temporary})
    add_test(
        NAME        run_icarus_${demo}_demo_compare
        COMMAND     ${CMAKE_COMMAND} -E compare_files "${reference_file}" "${output_file}"
    )
    set_tests_properties(
        run_icarus_${demo}_demo_compare
        PROPERTIES DEPENDS run_icarus_${demo}_demo_run
    )

endforeach()
