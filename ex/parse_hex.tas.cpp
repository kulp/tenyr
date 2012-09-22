#include "common.th"

_start:
    prologue

    c <- rel(string)
    d <- 0
    e <- 2
    call(strtol)
    illegal

string: .utf32 "1100"
