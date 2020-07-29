#include "common.th"

.set ARGUMENT, 10

_start:
    prologue
    c <- $ARGUMENT      // argument
    call(fib)
    illegal

// Computes fib(C) and stores the result in B.
fib:
    pushall(d,g,k)
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
    popall(d,g,k)
    ret
