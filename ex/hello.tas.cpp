#include "common.th"

_start:
    prologue
    c <- @+hi + p       // string starts at @hi
    call(puts)
    illegal

hi:
    .utf32 "hello, world"
    .word 0             // mark end of string with a zero

