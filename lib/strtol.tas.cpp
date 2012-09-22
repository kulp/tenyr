#include "common.th"

    .global strtol
// c,d,e <- str, endptr, base
strtol:
    pushall(f,g,h,i)

    h <- e == 0
    b <- e // in case we don't call
    callnz(h,detect_base)
    e <- b

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

// ----------------------------------------------------------------------------
// modifies C upon return
detect_base:
    push(f)
    f <- [c]
    f <- f == '0'
    jnzrel(f,detect_base_8or16)
    b <- 10
detect_base_done:
    pop(f)
    ret
detect_base_8or16:
    f <- [c + 1]
    f <- f &~ ('a' - 'A')
    f <- f == 'X'
    jnzrel(f,detect_base_16)
    b <- 8
    c <- c + 1
    goto(detect_base_done)
detect_base_16:
    b <- 16
    c <- c + 2
    goto(detect_base_done)

