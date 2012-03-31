#ifndef ARGUMENT
#define ARGUMENT 100
#endif

#include "common.th"

    f <- p - .          // load base
    b <- ARGUMENT       // b = upper limit (N)
    c <- b >> 1         // c = (N/2)
    i <- 2              // i = outer loop control
    o <- rel(array)     // base of array
    j <- 2              // j = inner index, setup index
    d <- -1             // d = truth

    a -> [o + 0]        // 0 is not prime
    a -> [o + 1]        // 1 is not prime
init:
    d -> [o + j]        // write truth to table cell
    j <- j + 1          // increment table index
    m <- j <= b         // exit loop when i > N
    jnzrel(m,init)
outer:
    m <- i > c          // exit loop when i > N/2
    jnzrel(m,done)
    g <- [o + i]        // load compositeness
    m <- g == 0         // check recorded compositeness
    jnzrel(m,bottom)    // branch if composite
    k <- 2              // set up initial multiplier
inner:
    j <- i * k          // compute j
    k <- k + 1          // increment multiplier
    m <- j > b          // compare j to N
    jnzrel(m,bottom)    // branch if j > N
    a -> [o + j]        // mark as composite
    goto(inner)         // loop

bottom:
    i <- i + 1          // check next candidate
    goto(outer)         // loop

done:
    illegal

    .word 0,0,0,0,0,0,0
    .word 0,0,0,0,0,0   // align array to 0x1030
array:
    .word 0

