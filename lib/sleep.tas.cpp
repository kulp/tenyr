#include "common.th"

.global sleep
sleep:
    // 80MHz clock, 4-cycle ticks() loop, 10cpi
    c <- c * 1000
    c <- c * 2000
    push(p + 2); p <- @+ticks + p
    o <- o + 1
    p <- [o]

ticks:
    o <- o - 2
    d -> [o + (2 - 0)]
    e -> [o + (2 - 1)]
    d <- 0
Lticks_loop:
    e <- d < c
    d <- d + 1
    a <- a // delay to make the loop 4 cycles long
    p <- @+Lticks_loop & e + p
    o <- o + 3
    e <- [o - (1 + 1)]
    d <- [o - (1 + 0)]
    p <- [o]

