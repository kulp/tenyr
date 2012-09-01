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
    f <- p - .          // base pointer
    c <- 1              // argument
    o <- 1023           // stack pointer
loop:
    push(c)
    call(fib)
    pop(c)
    c -> [0x100]
    //c -> [0x101]
    //b -> d
    push(c)
    c <- b
    call(say)
    pop(c)
    c <- c + 1
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

say:
    h <- 0x100
    h <- h << 8         // h is video base
    k <- 0x30           // k is offset into display (32)
    i <- rel(hexes)     // i is base of hex transform

sayloop:
    m <- h + k          // m is (k - 16) characters past start of text region
    l <- c == 0         // shall we loop ?
    jnzrel(l,saydone)
    j <- [c & 0xf + i]  // j is character for bottom 4 bits of c
    [m] <- j            // write character to display
    k <- k - 1          // go to the left one character
    c <- c >> 4         // shift down for next iteration
    goto(sayloop)

saydone:
    ret

hexes:
    .word '0', '1', '2', '3', '4', '5', '6', '7'
    .word '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'

