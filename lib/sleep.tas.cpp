#include "common.th"

sleep:
    push(d)
    c <- c * 2047
    c <- c * 2047
sleep_loop:
    c <- c - 1
    d <- c == 0
    jnzrel(d,sleep_done)
    goto(sleep_loop)
sleep_done:
    pop(d)
    ret

