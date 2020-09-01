#include "common.th"

#ifndef ARGUMENT
#define ARGUMENT 10
#endif

_start:
    o <- ((1 << 13) - 1)
    c <- ARGUMENT       // argument
    [o] <- p + 2 ; o <- o - 1 ; p <- @+fib + p
    illegal

// Computes fib(C) and stores the result in B.
fib:
    o <- o - 3
    d -> [o + (3 - 0)]
    g -> [o + (3 - 1)]
    k -> [o + (3 - 2)]
    b <- 0
    d <- 1

loop:
    k <- c == 0
    p <- @+done & k + p

    g <- b + d
    b <- d
    d <- g

    c <- c - 1
    p <- p + @+loop

done:
    o <- o + 4
    k <- [o - (1 + 2)]
    g <- [o - (1 + 1)]
    d <- [o - (1 + 0)]
    p <- [o]
