#include <common.th>

_start:
    prologue

    E <- 32
loop_outer:
    G <- E < 0
    P <- @+done & G + P
    D <- 0xa5 // TODO try other bit patterns
    C <- D << E
    call(trailz)
    G <- B == E
    E <- E - 1
    P <- @+skip &~ G + P
    C <- @+good_msg + P
    call(puts)
    P <- P + @+loop_outer
skip:
    C <- @+bad_msg + P
    call(puts)
    P <- P + @+loop_outer

done:
    illegal

good_msg: .utf32 "good\n" ; .word 0
bad_msg:  .utf32 "bad\n"  ; .word 0

