#include "common.th"

_start:
    prologue

    c <- @+this + p
    d <- @+that + p
    f <- @+strcmp + p
    call(cmp)

    c <- @+this + p
    d <- @+this + p
    f <- @+strcmp + p
    call(cmp)

    c <- @+this + p
    d <- @+that + p
    e <- 6
    f <- @+strncmp + p
    call(cmp)

    c <- @+this + p
    d <- @+that + p
    e <- 50
    f <- @+strncmp + p
    call(cmp)

    illegal

cmp:
    push(c)
    push(d)
    call(puts)      // output first string
    c <- @+nl + p   // followed by newline
    call(puts)
    pop(d)
    pop(c)

    push(c)
    push(d)
    c <- d
    call(puts)      // output second string
    c <- @+nl + p   // followed by newline
    call(puts)
    pop(d)
    pop(c)

    call(check)     // actually compare them

    c <- @+nl + p   // ... followed by newline
    call(puts)
    c <- @+nl + p   // and another
    call(puts)

    o <- o + 1
    p <- [o]

check:
    push(p + 2)
    p <- f          // call indirect
    p <- @+_mismatched & b + p
    c <- @+_match_message + p
    p <- p + @+_finished
_mismatched:
    c <- @+_mismatch_message + p
_finished:
    call(puts)
    o <- o + 1
    p <- [o]

_match_message:
    .chars "strings matched"
    .word 0
_mismatch_message:
    .chars "strings did not match"
    .word 0

this:
    .chars "this is a longish string"
    .word 0
that:
    .chars "this is a longish string2"
    .word 0
nl:
    .word '\n'
    .word 0

