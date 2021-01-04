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

