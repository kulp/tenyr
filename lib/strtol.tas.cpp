#include "common.th"

    .global strtol
// c,d,e <- str, endptr, base
strtol:
    pushall(f,g,h,i)
    b <- 0
    i <- [c]
    g <- i == '-'
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
    // TODO distinguish between partial conversion and no conversion
strtol_done:
    b <- b * f
    popall(f,g,h,i)
    ret

// ----------------------------------------------------------------------------
strtol_le10_top:
    i <- [c]
    c <- c + 1
    i <- i - '0'
    g <- i < e
    jzrel(g,strtol_done)
    g <- i > -1
    jzrel(g,strtol_done)
    b <- b * e
    b <- b + i
    goto(strtol_le10_top)

// ----------------------------------------------------------------------------
strtol_gt10_top:
    i <- [c]
    c <- c + 1
    g <- i - '0'
    h <- g > -1
    jzrel(h,strtol_done)
    h <- g < e
    jzrel(h,strtol_gt10_tryhigh)
    b <- b * e
    b <- b + g
    goto(strtol_gt10_top)

strtol_gt10_tryhigh:
    i <- i &~ ('a' - 'A')
    g <- i - 'A'
    h <- g > -1
    jzrel(h,strtol_done)
    g <- g + 10
    h <- g < e
    jzrel(h,strtol_done)
    b <- b * e
    b <- b + g
    goto(strtol_gt10_top)

