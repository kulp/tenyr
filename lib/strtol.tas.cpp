#include "common.th"
#include "errno.th"

    .global strtol
// c,d,e <- str, endptr, base
strtol:
    pushall(f,g,h,i)

    h <- e == 0
    b <- e // in case we don't call
    callnz(h,detect_base)
    e <- b

    h <- 36 < e
    jnzrel(h,strtol_error_EINVAL)
    h <- e < 2
    jnzrel(h,strtol_error_EINVAL)

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
    h -> errno
strtol_done:
    b <- b * f
    h <- d == 0
    jnzrel(h,strtol_no_endptr)
    c -> [d]
strtol_no_endptr:
    popall(f,g,h,i)
    ret

strtol_error_ERANGE:
    h <- ERANGE
    goto(strtol_error)

// TODO produce EINVAL
// only way I can think to do this so far is to get ilog2(base) and check for
// that many zero bits at the MSB end before every multiplication
strtol_error_EINVAL:
    h <- EINVAL
    goto(strtol_error)

// ----------------------------------------------------------------------------
strtol_le10_top:
    i <- [c]
    i <- i - '0'
    g <- i < e
    jzrel(g,strtol_done)
    g <- i >= 0
    jzrel(g,strtol_done)
    c <- c + 1
    b <- b * e
    b <- b + i
    goto(strtol_le10_top)

// ----------------------------------------------------------------------------
strtol_gt10_top:
    i <- [c]
    g <- i - '0'
    h <- g >= 0
    jzrel(h,strtol_done)
    h <- g < e
    jzrel(h,strtol_gt10_tryhigh)
    c <- c + 1
    b <- b * e
    b <- b + g
    goto(strtol_gt10_top)

strtol_gt10_tryhigh:
    i <- i &~ ('a' - 'A')
    g <- i - 'A'
    h <- g >= 0
    jzrel(h,strtol_done)
    g <- g + 10
    h <- g < e
    jzrel(h,strtol_done)
    c <- c + 1
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

