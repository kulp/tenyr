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
    prologue            // sets up base/stack pointer
    c <- ARGUMENT       // argument
    call(fib)
    illegal

fib:
    d <- 1
    d <- c > d          // zero or one ?
    jnzrel(d,_recurse)  // not-zero is true (c >= 2)
    b <- c
    ret

_recurse:
    push(c)
    c <- c - 1
    call(fib)
    pop(c)
    push(b)
    c <- c - 2
    call(fib)
    d <- b
    pop(b)
    b <- d + b

    ret

