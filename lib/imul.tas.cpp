#include "common.th"

// c <- multiplicand
// d <- multiplier
// b -> product
.global imul
imul:
    pushall(h,i,j,k)

    i <- d == 0
    jnzrel(i, L_done)
    h <- 1
    b <- 0

    j <- c >> 31    // save sign bit in j
    j <- -j         // convert sign to flag
    c <- c ^ j      // adjust multiplicand
    c <- c - j

    k <- d >> 31    // save sign bit in k
    k <- -k         // convert sign to flag
    d <- d ^ k      // adjust multiplier
    d <- d - k

    j <- j ^ k      // final flip flag is in j

L_top:
    // use constant 1 in h to combine instructions
    i <- d & h - 1
    i <- c &~ i
    b <- b + i
    c <- c << 1
    d <- d >> 1
    i <- d <> 0
    jnzrel(i, L_top)

L_done:
    b <- b ^ j      // adjust product for signed math
    b <- b - j

    popall(h,i,j,k)
    ret

