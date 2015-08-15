#include "common.th"

    b <- -0x10000
    b -> [0x201] // read-write
    d <- -1
    d -> [0x200] // enable
    c <- 0
top:
    // 7-segment mirror
    d <- c >> 16
    d -> [0x100]
    d -> [0x101]

    c -> [0x203]

    e <- [0x202]
    e <- e & 0xff
    e <- e << 16
    c <- e | c + 1
goto(top)

illegal

