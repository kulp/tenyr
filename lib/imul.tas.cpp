#include "common.th"

// c <- multiplicand
// d <- multiplier
// b -> product
.global imul
imul:
    o <- o - 3
    h -> [o + (3 - 0)]
    i -> [o + (3 - 1)]
    j -> [o + (3 - 2)]

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
    p <- @+L_top &~ i + p

    b <- b ^ j      // adjust product for signed math
    b <- b - j

    o <- o + 3
    j <- [o - 2]
    i <- [o - 1]
    h <- [o - 0]
L_done:
    o <- o + 1
    p <- [o]

