#include "common.th"

print_num:
    ret

print_num_generic:
    pushall(d,f,g,h)
    g <- c
    h <- rel(tmpbuf_end)
    d <- c < 0
    jnzrel(d,print_num_generic_negative)
    f <- 0
print_num_generic_top:
    h <- h - 1

    c <- g
    d <- 10
    call(umod)

    [h] <- b + '0'

    c <- g
    d <- 10
    call(udiv)
    g <- b

    d <- g == 0
    jzrel(d,print_num_generic_top)
    jzrel(f,print_num_generic_done)
    h <- h - 1
    [h] <- '-'

print_num_generic_done:
    b <- h
    popall(d,f,g,h)
    ret
print_num_generic_negative:
    g <- - g
    f <- -1
    goto(print_num_generic_top)

tmpbuf: .utf32 "0123456789abcdef"
tmpbuf_end: .word 0

