#include "common.th"

_start:
    prologue
    c <- @+dst + p
    d <- @+src + p
    e <- 10
    call(memcpy)
    c <- b
    call(puts)
    illegal

dst: .utf32 "                " ; .word 0
src: .utf32 "0123456789ABCDEF" ; .word 0

