#include <common.th>

.global trailz
trailz:
    O <- O - 3
    D -> [O + (3 - 0)]
    E -> [O + (3 - 1)]
    F -> [O + (3 - 2)]
    // get lowest set bit in C
    B   <- C - 1
    B   <- C ^ B
    C   <- C & B
    // now only one bit is set in C if C was nonzero
    // if C was zero, C is still zero, and we exit early
    D   <- C == 0
    B   <- D & 32
    P   <- @+done & D + P

    // D is the building register, moved to B at the end
    E   <- 0xffff       // E,F are mask registers

    // if we find only zeros in one half, the other half has a bit set
    B   <- C &~ E       // B <- 16 MSb of C
    D   <- B == A + 1   // D <- (16 MSb zero) ? 0 : 1

    F   <- E << 8       // F <- 0x00ffff00
    F   <- E ^  F       // F <- 0x00ff00ff
    B   <- C &~ F       // B <- 8 MSb of C
    B   <- B == A + 1   // B <- (8 MSb zero) ? 0 : 1
    D   <- D << 1 + B   // shift and add

    E   <- F << 4       // E <- 0x0ff00ff0
    E   <- F ^  E       // E <- 0x0f0f0f0f
    B   <- C &~ E       // 00001111
    B   <- B == A + 1   // B <- (4 MSb zero) ? 0 : 1
    B   <- D << 1 + B

    F   <- E << 2       // F <- 0x3c3c3c3c
    F   <- E ^  F       // F <- 0x33333333
    D   <- C &~ F
    D   <- D == A + 1
    D   <- B << 1 + D

    E   <- F << 1       // E <- 0x66666666
    E   <- F ^  E       // E <- 0x55555555
    B   <- C &~ E
    B   <- B == A + 1
    B   <- D << 1 + B

done:
    O <- O + 4
    F <- [O - (1 + 2)]
    E <- [O - (1 + 1)]
    D <- [O - (1 + 0)]
    P <- [O]

