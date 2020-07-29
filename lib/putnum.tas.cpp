#include "common.th"
#include "vga.th"

.global putnum
    // putnum takes number in C, (row,col) in D,E, and field width in F
putnum:
    o <- o - 6
    h -> [o + (6 - 0)]
    i -> [o + (6 - 1)]
    j -> [o + (6 - 2)]
    k -> [o + (6 - 3)]
    l -> [o + (6 - 4)]
    m -> [o + (6 - 5)]
    h <- 0x10000        // h is video base
    k <- d * COLS + e   // k is offset into display
    k <- k + f          // start at right side of field
    i <- @+hexes + p    // i is base of hex transform

putnumloop:
    j <- [c & 0xf + i]  // j is character for bottom 4 bits of c
    j -> [h + k]        // write character to display
    k <- k - 1          // go to the left one character
    c <- c >>> 4        // shift down for next iteration
    l <- c == 0         // shall we loop ?
    p <- @+putnumloop &~ l + p

putnumdone:
    o <- o + 7
    m <- [o - (1 + 5)]
    l <- [o - (1 + 4)]
    k <- [o - (1 + 3)]
    j <- [o - (1 + 2)]
    i <- [o - (1 + 1)]
    h <- [o - (1 + 0)]
    p <- [o]

hexes:
    .word '0', '1', '2', '3', '4', '5', '6', '7'
    .word '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'

