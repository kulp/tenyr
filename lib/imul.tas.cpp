#include "common.th"

// c <- multiplicand
// d <- multiplier
// b -> product
.global imul
imul:
    pushall(h,i)

    i <- d == 0
    jnzrel(i, L_done)

L_top:
    h <- d & 1
    i <- h <> 0
    i <- c & i
    b <- b + i
    c <- c << 1
    d <- d >> 1
    i <- d <> 0
    jnzrel(i, L_top)

L_done:
    popall(h,i)
    ret

