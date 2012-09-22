#include "common.th"

_start:
    prologue

    c <- rel(string_oct)
    d <- 0
    e <- 0
    call(strtol)

    c <- rel(string_dec)
    d <- 0
    e <- 0
    call(strtol)

    c <- rel(string_hex)
    d <- 0
    e <- 0
    call(strtol)

    c <- rel(string_36)
    d <- 0
    e <- 36
    call(strtol)

    illegal

string_dec: .utf32 "123" ; .word 0
string_oct: .utf32 "0123" ; .word 0
string_hex: .utf32 "0x123" ; .word 0
string_36:  .utf32 "1Za" ; .word 0
