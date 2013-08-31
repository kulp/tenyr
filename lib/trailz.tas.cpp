#include <common.th>

.global trailz
trailz:
    pushall(D,E,F)
    // get lowest set bit in C
    B   <- C - 1
    B   <- C ^ B
    C   <- C & B

    D   <- 0            // D is the building register, moved to B at the end
    F   <- 0xff         // E,F are mask registers
    E   <- F <<  8 + F  // E <- 0x0000ffff
    F   <- F << 16 + F  // F <- 0x00ff00ff

    B   <- C & E        // B <- high 16 bits of C
    B   <- B <> A + 1   // B <- (high 16 bits nonzero) ? 1 : 0
    D   <- D << 1 + B   // D <- ((high 16 bits nonzero) ? 1 : 0) + (D << 1)

    B   <- C & F        // B <- high 8 bits of C
    B   <- B <> A + 1   // B <- (high 8 bits nonzero) ? 1 : 0
    D   <- D << 1 + B   // D <- ((high 8 bits nonzero) ? 1 : 0) + (D << 1)

    B   <- C & 0x0f     // 00001111
    B   <- B <> A + 1   // B <- (set bit is in bottom half) ? 0 : 1
    B   <- D << 1 + B

    D   <- C & 0x33     // 00110011
    D   <- D <> A + 1
    D   <- B << 1 + D

    B   <- C & 0x55     // 01010101
    B   <- B <> A + 1
    B   <- D << 1 + B

    popall(D,E,F)
    ret

