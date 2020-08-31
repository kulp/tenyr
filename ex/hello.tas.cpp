#include "common.th"

_start:
    prologue
    c <- @+hi + p       // string starts at @hi
    push(p + 2); p <- @+puts + p
    illegal

hi:
    .chars "hello, world"
    .word 0             // mark end of string with a zero

