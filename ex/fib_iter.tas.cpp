#include "common.th"

#ifndef ARGUMENT
#define ARGUMENT 10
#endif

_start:
    f <- p - .          // base pointer
    c <- ARGUMENT       // argument
    o <- -1             // stack pointer
    call(fib)
    illegal

// Computes fib(C) and stores the result in B.
fib:
    f <- p - .
    b <- 0
    d <- 1

loop:
    k <- c == 0
    jnzrel(k, done)

    g <- b + d
    b <- d
    d <- g

    c <- c - 1
    goto(loop)

done:
    ret
