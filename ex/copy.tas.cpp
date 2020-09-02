#include "common.th"

_start:
    prologue
    c <- rel(dst)
    d <- rel(src)
    e <- 10
    call(memcpy)
    c <- b
    call(puts)
    illegal

dst: .chars "                " ; .word 0
src: .chars "0123456789ABCDEF" ; .word 0

