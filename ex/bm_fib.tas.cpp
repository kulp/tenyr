// Conventions:
//  O is the stack pointer (post-decrement)
//  B is the (so far only) return register
//  C is the (so far only) argument register
//  N is the relative-jump temp register

#include "common.th"

_start:
    bare_metal_init()   // TODO this shouldn't be necessary
    c <- 1              // argument
    prologue

    //call(init_display)
    //call(disable_cursor)

    j <- 0              // row (0 - 39)
    k <- 0              // column (0 - 3)

loop:
    l <- k * 20         // column (0 - 79)

    // compute fib(N)
    push(c)
    call(fib)
    pop(c)

    c -> [0x100]        // write N to LED
    //c -> [0x101]

    // write N to VGA
    push(c)
    d <- j
    e <- l
    f <- 2
    call(putnum)
    pop(c)

    // write fib(N) to VGA
    push(c)
    c <- b
    d <- j
    e <- l + 4
    f <- 12
    call(putnum)
    pop(c)

    c <- c + 1          // increment N

    j <- j + 1          // increment row
    m <- j > 39         // check for column full
    // TODO try `l <- m * -20 + l`
    k <- k - m          // if j > 39, k <- k - -1
    j <- j &~ m         // if j > 39, j <- j & 0

    goto(loop)
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

