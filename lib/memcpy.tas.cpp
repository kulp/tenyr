#include "common.th"
#include "errno.th"

    .global memcpy
// c,d,e <- dst, src, len
memcpy:
    pushall(f,g)
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
    popall_ret(f,g)

