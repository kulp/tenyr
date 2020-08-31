// Conventions:
//  O is the stack pointer (post-decrement)
//  B is the (so far only) return register
//  C is the (so far only) argument register
//  N is the relative-jump temp register

#include "common.th"
#include "vga.th"

_start:
    c <- 1              // argument
    prologue

    push(p + 2); p <- @+init_display + p

    j <- 0              // row (0 - 31)
    k <- 0              // column (0 - 3)

loop:
    l <- k * 16         // column (0 - 63)

    // compute fib(N)
    push(c)
    push(p + 2); p <- @+fib + p
    pop(c)

    c -> [0x100]        // write N to LED
    //c -> [0x101]

    // write N to VGA
    push(c)
    d <- j
    e <- l
    f <- 2
    push(p + 2); p <- @+putnum + p
    pop(c)

    // write fib(N) to VGA
    push(c)
    c <- b
    d <- j
    e <- l + 4
    f <- 12
    push(p + 2); p <- @+putnum + p
    pop(c)

    c <- c + 1          // increment N

    j <- j + 1          // increment row
    m <- j >= ROWS      // check for column full
    // TODO try `l <- m * -20 + l`
    k <- k - m          // if j >= ROWS, k <- k - -1
    j <- j &~ m         // if j >= ROWS, j <- j & 0

    p <- p + @+loop
    illegal

fib:
    d <- 1
    d <- d < c          // zero or one ?
    p <- @+_recurse & d + p  // not-zero is true (c >= 2)
    b <- c
    o <- o + 1
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

    o <- o + 1
    p <- [o]

