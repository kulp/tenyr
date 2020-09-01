// Conventions:
//  O is the stack pointer (post-decrement)
//  B is the (so far only) return register
//  C is the (so far only) argument register
//  N is the relative-jump temp register

#ifndef ARGUMENT
#define ARGUMENT 10
#endif

#include "common.th"

_start:
    prologue                // sets up base/stack pointer
    c <- ARGUMENT           // argument
    [o] <- p + 2 ; o <- o - 1 ; p <- @+fib + p
    illegal

fib:
    [o] <- d ; o <- o - 1
    d <- c > 1              // not zero or one ?
    p <- @+_recurse & d + p // not-zero is true (c >= 2)
    b <- c
    o <- o + 2
    d <- [o - (1 + 0)]
    p <- [o]

_recurse:
    [o] <- c ; o <- o - 1
    c <- c - 1
    [o] <- p + 2 ; o <- o - 1 ; p <- @+fib + p
    o <- o + 1 ; c <- [o]
    [o] <- b ; o <- o - 1
    c <- c - 2
    [o] <- p + 2 ; o <- o - 1 ; p <- @+fib + p
    d <- b
    o <- o + 1 ; b <- [o]
    b <- d + b

    o <- o + 2
    d <- [o - (1 + 0)]
    p <- [o]

