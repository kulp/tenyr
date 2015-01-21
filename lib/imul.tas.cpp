#include "common.th"

// c <- multiplicand
// d <- multiplier
// b -> product
.global imul
imul:
#if IMUL_EARLY_EXITS
    b <- c == 0
    d <- d &~ b     // d = (c == 0) ? 0 : d
    b <- d == 0
    jnzrel(b, L_done)
#endif

    pushall(h,i,j)

    b <- 0
    h <- 1

    j <- d >> 31    // save sign bit in j
    d <- d ^ j      // adjust multiplier
    d <- d - j

L_top:
    // use constant 1 in h to combine instructions
    i <- d & h - 1
    i <- c &~ i
    b <- b + i
    c <- c << 1
    d <- d >> 1
    i <- d == 0
    jzrel(i, L_top)

    b <- b ^ j      // adjust product for signed math
    b <- b - j

    popall(h,i,j)
L_done:
    ret

