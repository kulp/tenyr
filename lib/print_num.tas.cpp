#include "common.th"

print_num:
    ret

print_num_generic:
    pushall(d,f,g,h)
    g <- c
    h <- @+tmpbuf_end + p
    d <- c < 0
    p <- @+print_num_generic_negative & d + p
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
    p <- @+print_num_generic_top &~ d + p
    p <- @+print_num_generic_done &~ f + p
    h <- h - 1
    [h] <- '-'

print_num_generic_done:
    b <- h
    popall_ret(d,f,g,h)
print_num_generic_negative:
    g <- - g
    f <- -1
    p <- p + @+print_num_generic_top

tmpbuf: .utf32 "0123456789abcdef"
tmpbuf_end: .word 0

