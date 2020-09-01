#include "common.th"

_start:
    prologue

top:
    d <- [@+min0 + p]
    e <- [@+min1 + p]
    f <- [@+sec0 + p]
    g <- [@+sec1 + p]

    h <- d << 4 + e
    h <- h << 4 + f
    h <- h << 4 + g
    h -> [0x100]
    h -> [0x101]

    c <- 1
    [o] <- p + 2 ; o <- o - 1 ; p <- @+sleep + p

    g <- g + 1
    h <- g == 10
    g <- g &~ h
    f <- f - h
    h <- f == 6
    f <- f &~ h
    e <- e - h
    h <- e == 10
    e <- e &~ h
    d <- d - h
    h <- d == 6
    d <- d &~ h

    d -> [@+min0 + p]
    e -> [@+min1 + p]
    f -> [@+sec0 + p]
    g -> [@+sec1 + p]
    p <- p + @+top

    illegal

min0: .word 0
min1: .word 0
sec0: .word 0
sec1: .word 0

