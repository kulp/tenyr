#include "common.th"

_start:
    prologue

    c <- @+string_oct + p
    d <- 0
    e <- 0
    call(strtol)

    c <- @+string_dec + p
    d <- 0
    e <- 0
    call(strtol)

    c <- @+string_hex + p
    d <- 0
    e <- 0
    call(strtol)

    c <- @+string_36 + p
    d <- 0
    e <- 36
    call(strtol)

    c <- @+string_gbg + p
    d <- 0
    e <- 10
    call(strtol)

    c <- @+string_gbg + p
    d <- @+next + p
    e <- 10
    call(strtol)

    c <- [@+next + p]
    d <- @+next + p
    e <- 16
    call(strtol)

    c <- @+string_gb2 + p
    d <- @+next + p
    e <- 8
    call(strtol)

    c <- [@+next + p]
    d <- @+next + p
    e <- 36
    call(strtol)

    illegal

next: .word 0

string_dec: .utf32 "123"   ; .word 0
string_oct: .utf32 "0123"  ; .word 0
string_hex: .utf32 "0x123" ; .word 0
string_36:  .utf32 "1Za"   ; .word 0
string_gbg: .utf32 "123CS" ; .word 0
string_gb2: .utf32 "679Z0" ; .word 0
