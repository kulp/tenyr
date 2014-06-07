// Conventions:
//  O is the stack pointer (post-decrement)
//  B is the (so far only) return register
//  C is the (so far only) argument register
//  N is the relative-jump temp register

#include "common.th"

#define bare_metal_init() \
    b <- a ; c <- a ; d <- a ; e <- a ; f <- a ; g <- a ; h <- a ; \
    i <- a ; j <- a ; k <- a ; l <- a ; m <- a ; n <- a ; o <- a ; \
    //

_start:
    bare_metal_init()   // TODO this shouldn't be necessary
    c <- 1              // argument
    o <- 1              // stack pointer
    o <- o << 13
    o <- o - 1

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
    illegal


init_display:
    pushall(h,k,l,m);
    h <- 0x100
    h <- h << 8         // h is video base
    k <- 0              // k is offset into text region

init_display_loop:
    m <- h + k + 0x10   // m is k characters past start of text region
    [m] <- ' '          // write space to display
    k <- k + 1          // go to the right one character
    l <- k < 0x21       // shall we loop ?
    jnzrel(l,init_display_loop)

init_display_done:
    popall(h,k,l,m);
    ret


disable_cursor:
    pushall(g,h)
    h <- 0x100
    h <- h << 8
    g <- [h]
    [h] <- g &~ (1 << 6)  // unset cursor-enable bit
    popall(g,h)
    ret


