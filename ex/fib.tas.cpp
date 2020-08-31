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
    push(p + 2); p <- @+fib + p
    illegal

fib:
    push(d)
    d <- c > 1              // not zero or one ?
    p <- @+_recurse & d + p // not-zero is true (c >= 2)
    b <- c
    o <- o + 2
    d <- [o - (1 + 0)]
    p <- [o]

_recurse:
    push(c)
    c <- c - 1
    push(p + 2); p <- @+fib + p
    pop(c)
    push(b)
    c <- c - 2
    push(p + 2); p <- @+fib + p
    d <- b
    pop(b)
    b <- d + b

    o <- o + 2
    d <- [o - (1 + 0)]
    p <- [o]

