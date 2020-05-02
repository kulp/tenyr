#include "common.th"

.global sleep
sleep:
    // 80MHz clock, 4-cycle ticks() loop, 10cpi
    c <- c * 1000
    c <- c * 2000
    call(ticks)
    o <- o + 1
    p <- [o]

ticks:
    pushall(d,e)
    d <- 0
Lticks_loop:
    e <- d < c
    d <- d + 1
    a <- a // delay to make the loop 4 cycles long
    p <- @+Lticks_loop & e + p
    popall_ret(d,e)

