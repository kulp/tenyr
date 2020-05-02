#include "common.th"
#include "errno.th"

    .global memset
// c,d,e <- dst, val, len
memset:
    pushall(f,g)
    b <- c                      // return original value of dst
    f <- 0                      // load offset with 0
L_memset_loop:
    g <- e == f                 // check if count is reached
    p <- @+L_memset_done & g + p
    d -> [c + f]                // store val into dst word
    f <- f + 1                  // increment offset
    p <- p + @+L_memset_loop
L_memset_done:
    popall(f,g)
    ret

