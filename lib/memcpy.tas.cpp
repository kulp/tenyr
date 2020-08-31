#include "common.th"

    .global memcpy
// c,d,e <- dst, src, len
memcpy:
    o <- o - 2
    f -> [o + (2 - 0)]
    g -> [o + (2 - 1)]
    b <- c                      // return original value of dst
    f <- 0                      // load offset with 0
L_memcpy_loop:
    g <- e == f                 // check if count is reached
    p <- @+L_memcpy_done & g + p
    g <- [d + f]                // copy src word into temp
    g -> [c + f]                // copy temp into dst word
    f <- f + 1                  // increment offset
    p <- p + @+L_memcpy_loop
L_memcpy_done:
    o <- o + 3
    g <- [o - (1 + 1)]
    f <- [o - (1 + 0)]
    p <- [o]

