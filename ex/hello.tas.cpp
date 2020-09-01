#include "common.th"

_start:
    o <- ((1 << 13) - 1)
    c <- @+hi + p       // string starts at @hi
    [o] <- p + 2 ; o <- o - 1 ; p <- @+puts + p
    illegal

hi:
    .chars "hello, world"
    .word 0             // mark end of string with a zero

