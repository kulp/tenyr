#include <common.th>

_start:
    o <- ((1 << 13) - 1)

    E <- 32
loop_outer:
    G <- E < 0
    P <- @+done & G + P
    D <- 0xa5 // TODO try other bit patterns
    C <- D << E
    [o] <- p + 2 ; o <- o - 1 ; p <- @+trailz + p
    G <- B == E
    E <- E - 1
    P <- @+skip &~ G + P
    C <- @+good_msg + P
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    P <- P + @+loop_outer
skip:
    C <- @+bad_msg + P
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    P <- P + @+loop_outer

done:
    illegal

good_msg: .chars "good\n" ; .word 0
bad_msg:  .chars "bad\n"  ; .word 0

