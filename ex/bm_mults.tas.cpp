// Conventions:
//  O is the stack pointer (post-decrement)
//  B is the (so far only) return register
//  C is the (so far only) argument register
//  N is the relative-jump temp register

#include "common.th"

_start:
    prologue
    b <- 0              // indicate non-completion to testbench
    c <- 1              // argument

    call(init_display)
    call(disable_cursor)

    j <- 0              // multiplier (0 -> 0x26)
    k <- 0              // multiplicand (0 -> 0x12)

    c <- 0b0100         // decimal point between J and K
    c -> [0x101]

    // write the top row first
loop_top:
    f <- 4              // field width
    e <- k * f + 3      // column is multiplicand * field width + header width
    d <- 0              // row is 0
    c <- k              // number to print is multiplicand
    call(putnum)
    k <- k + 1
    c <- k < 0x13
    jnzrel(c,loop_top)

    // outer loop is j (multiplier, corresponds to row)
loop_j:
    k <- 0
    // print row header
    f <- 2              // field width 2
    e <- 0              // column is 0
    d <- j + 1          // row (J + 1)
    c <- j
    call(putnum)

loop_k:
    f <- 4              // field width 3
    e <- k * f + 3      // E is column (0 - 79)
    d <- j + 1
    c <- j * k
    call(putnum)
    c <- 0x100          // write {J,K} to LEDs
    [c] <- j << 8 + k
    k <- k + 1
    c <- k < 0x13
    jnzrel(c,loop_k)

    j <- j + 1          // increment N
    c <- j < 0x27
    jnzrel(c,loop_j)
    b <- -1             // indicate completion to testbench
    illegal

