#include <common.th>

_start:
    prologue

    E <- 32
loop_outer:
    G <- E < 0
    jnzrel(G,done)
    D <- 0xa5 // TODO try other bit patterns
    C <- D << E
    call(trailz)
    G <- B == E
    E <- E - 1
    jzrel(G,skip)
    C <- rel(good_msg)
    call(puts)
    goto(loop_outer)
skip:
    C <- rel(bad_msg)
    call(puts)
    goto(loop_outer)

done:
    illegal

good_msg: .utf32 "good\n" ; .word 0
bad_msg:  .utf32 "bad\n"  ; .word 0

