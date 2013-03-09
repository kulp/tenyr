#include "common.th"

// c <- multiplicand
// d <- multiplier
// b -> product
.global imul
imul:
    pushall(h,i,j)

    b <- 0
    i <- d == 0
    i <- c == 0 + i
    i <- i <> 0
    jnzrel(i, L_done)

    h <- 1

    j <- d >> 31    // save sign bit in j
    j <- -j         // convert sign to flag
    d <- d ^ j      // adjust multiplier
    d <- d - j

L_top:
    // use constant 1 in h to combine instructions
    i <- d & h - 1
    i <- c &~ i
    b <- b + i
    c <- c << 1
    d <- d >> 1
    i <- d <> 0
    jnzrel(i, L_top)

    b <- b ^ j      // adjust product for signed math
    b <- b - j

L_done:
    popall(h,i,j)
    ret

