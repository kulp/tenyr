#include "common.th"
#define newline 

_start:
    f <- p - .
    o <- -1                     // set up stack

    c <- rel(this)
    d <- rel(that)
    call(cmp)

    c <- rel(this)
    d <- rel(this)
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
    call(strcmp)
    jnzrel(b,_mismatched)
    c <- rel(_match_message)
    goto(_finished)
_mismatched:
    c <- rel(_mismatch_message)
_finished:
    call(puts)
    ret

_match_message:
    .ascii "strings matched"
    .word 0
_mismatch_message:
    .ascii "strings did not match"
    .word 0

this:
    .ascii "this is a longish string"
    .word 0
that:
    .ascii "this is a longish string2"
    .word 0
nl:
    .word 0x0a   // newline
    .word 0

