#ifndef ARGUMENT
#define ARGUMENT 100
#endif

#include "common.th"

    b <- ARGUMENT       // b = upper limit (N) (31-bit unsigned)
    c <- b >> 1         // c = (N/2)
    i <- 2              // i = outer loop control
    o <- @+array + p    // base of array
    j <- 2              // j = inner index, setup index
    d <- -1             // d = truth

    a -> [o + 0]        // 0 is not prime
    a -> [o + 1]        // 1 is not prime
init:
    d -> [o + j]        // write truth to table cell
    j <- j + 1          // increment table index
    m <- b < j          // exit loop when i > N
    jzrel(m,init)
outer:
    m <- c < i          // exit loop when i > N/2
    jnzrel(m,done)
    g <- [o + i]        // load compositeness
    m <- g == 0         // check recorded compositeness
    jnzrel(m,bottom)    // branch if composite
    k <- 2              // set up initial multiplier
inner:
    j <- i * k          // compute j
    k <- k + 1          // increment multiplier
    m <- b < j          // compare j to N
    jnzrel(m,bottom)    // branch if j > N
    a -> [o + j]        // mark as composite
    p <- p + @+inner    // loop

bottom:
    i <- i + 1          // check next candidate
    p <- p + @+outer    // loop

done:
    illegal

    .word 0,0,0,0,0     // align array to 0x20
array:
    .word 0

