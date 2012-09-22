#include "common.th"

    .global strtol
// c,d,e <- str, endptr, base
strtol:
    pushall(d,f,g)
    b <- 0
    d <- [c]
    g <- d == '-'
    jnzrel(g,strtol_negative)
    f <- 1
strtol_top:
    g <- e < 11
    jnzrel(g,strtol_le10_top)
    goto(strtol_gt10_top)

strtol_negative:
    f <- -1
    c <- c + 1
    goto(strtol_top)

strtol_error:
    // TODO
strtol_done:
    b <- b * f
    popall(d,f,g)
    ret

// ----------------------------------------------------------------------------
strtol_le10_top:
    d <- [c]
    c <- c + 1
    d <- d - '0'
    g <- d < e
    jzrel(g,strtol_error)
    g <- d > -1
    jzrel(g,strtol_done)
    b <- b * e
    b <- b + d
    goto(strtol_le10_top)

// ----------------------------------------------------------------------------
strtol_gt10_top:
    illegal

