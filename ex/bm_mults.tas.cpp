// Conventions:
//  O is the stack pointer (post-decrement)
//  B is the (so far only) return register
//  C is the (so far only) argument register
//  N is the condition register

#include "common.th"
#include "vga.th"

_start:
    prologue
    b <- 0              // indicate non-completion to testbench
    c <- 1              // argument

    call(init_display)

restart:
    j <- 0              // multiplier
    k <- 0              // multiplicand
    g <- 2              // screen columns consumed

    c <- 0b0100         // decimal point between J and K
    c -> [0x101]

    // write the top row first
loop_top:
    f <- 4              // field width
    n <- k <= (0x100 / (ROWS - 2))
    f <- f + n          // conditionally shrink field width
    e <- k + g          // column is multiplicand + filled width
    d <- 0              // row is 0
    c <- k              // number to print is multiplicand
    call(putnum)
    k <- k + 1
    g <- g + f - 1
    c <- k <= 16
    p <- @+loop_top & c + p

    // outer loop is j (multiplier, corresponds to row)
loop_j:
    k <- 0
    g <- 0
    // print row header
    f <- 2              // field width
    e <- 0              // column is 0
    d <- j + 1          // row (J + 1)
    c <- j
    call(putnum)
    g <- g + f

loop_k:
    f <- 4              // field width
    n <- k <= (0x100 / (ROWS - 2))
    f <- f + n          // conditionally shrink field width
    e <- k + g          // column is multiplicand + filled width
    d <- j + 1
    c <- j * k
    call(putnum)
    c <- 0x100          // write {J,K} to LEDs
    [c] <- j << 8 + k
    k <- k + 1
    g <- g + f - 1
    c <- k <= 16
    p <- @+loop_k & c + p

    j <- j + 1          // increment N
    c <- j < ROWS
    p <- @+loop_j & c + p
    b <- -1             // indicate completion to testbench

    //p <- p + @+restart     // restartable, but exits to testbench by default
    illegal

