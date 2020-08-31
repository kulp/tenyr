#include "common.th"

print_num:
    o <- o + 1
    p <- [o]

print_num_generic:
    o <- o - 4
    d -> [o + (4 - 0)]
    f -> [o + (4 - 1)]
    g -> [o + (4 - 2)]
    h -> [o + (4 - 3)]
    g <- c
    h <- @+tmpbuf_end + p
    d <- c < 0
    p <- @+print_num_generic_negative & d + p
    f <- 0
print_num_generic_top:
    h <- h - 1

    c <- g
    d <- 10
    push(p + 2); p <- @+umod + p

    [h] <- b + '0'

    c <- g
    d <- 10
    push(p + 2); p <- @+udiv + p
    g <- b

    d <- g == 0
    p <- @+print_num_generic_top &~ d + p
    p <- @+print_num_generic_done &~ f + p
    h <- h - 1
    [h] <- '-'

print_num_generic_done:
    b <- h
    o <- o + 5
    h <- [o - (1 + 3)]
    g <- [o - (1 + 2)]
    f <- [o - (1 + 1)]
    d <- [o - (1 + 0)]
    p <- [o]
print_num_generic_negative:
    g <- - g
    f <- -1
    p <- p + @+print_num_generic_top

tmpbuf: .chars "0123456789abcdef"
tmpbuf_end: .word 0

