#include "common.th"

_start:
    prologue

    c <- rel(this)
    d <- rel(that)
    f <- rel(strcmp)
    call(cmp)

    c <- rel(this)
    d <- rel(this)
    f <- rel(strcmp)
    call(cmp)

    c <- rel(this)
    d <- rel(that)
    e <- 6
    f <- rel(strncmp)
    call(cmp)

    c <- rel(this)
    d <- rel(that)
    e <- 50
    f <- rel(strncmp)
    call(cmp)

    illegal

cmp:
    push(c)
    push(d)
    call(puts)      // output first string
    c <- rel(nl)    // followed by newline
    call(puts)
    pop(d)
    pop(c)

    push(c)
    push(d)
    c <- d
    call(puts)      // output second string
    c <- rel(nl)    // followed by newline
    call(puts)
    pop(d)
    pop(c)

    call(check)     // actually compare them

    c <- rel(nl)    // ... followed by newline
    call(puts)
    c <- rel(nl)    // and another
    call(puts)

    ret

check:
    callr(f)
    jnzrel(b,_mismatched)
    c <- rel(_match_message)
    goto(_finished)
_mismatched:
    c <- rel(_mismatch_message)
_finished:
    call(puts)
    ret

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

