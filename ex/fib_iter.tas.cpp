#include "common.th"

#ifndef ARGUMENT
#define ARGUMENT 10
#endif

_start:
    prologue
    c <- ARGUMENT       // argument
    call(fib)
    illegal

// Computes fib(C) and stores the result in B.
fib:
    pushall(d,g,k)
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
    popall(d,g,k)
    ret
