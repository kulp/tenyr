set(libfiles
    abort
    bsearch
    gcd
    imul
    ipow
    isqrt
    memcpy
    memset
    print_hex
    print_num
    putnum
    puts
    qsort
    rand
    sleep
    strcmp
    strtol
    trailz
    udiv
    umod
)

foreach(f ${libfiles})

    add_library(libtenyr_${f} OBJECT ${f}.tas)
    list(APPEND libtenyr_OBJECTS ${f}.tas)

endforeach()

add_library(libtenyr OBJECT ${libtenyr_OBJECTS})

