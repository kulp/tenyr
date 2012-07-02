#include "forth_common.th"

searchterm: .utf32 "OVER" ; .word 0

.set link, 0
head(start,start):
    //.word @FLOOP
    .word
    //@WORDS,
    @LIT,
    @searchterm,
    @TICK,

    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,

    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,

    //@DUP, @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT, @LIT, 4, @RSHIFT,
    //@DUP, @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT, @LIT, 4, @RSHIFT,
    //@DUP, @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT, @LIT, 4, @RSHIFT,
    //@DUP, @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT, @LIT, 4, @RSHIFT,

    @CR

    .word @EXIT

head(FLOOP,FLOOP):
    .word @ENTER
    .word @BLOOP
    .word @BLOOP
    .word @BLOOP
    .word @EXIT
    illegal

head(BLOOP,BLOOP):
    .word @ENTER
    .word @NOOP
    .word @NOOP
    .word @LIT, 65
    .word @DUP
    .word @EMIT
    .word @LIT, 32
    .word @ADD
    .word @EMIT
    .word @LIT, 10
    .word @EMIT
    .word @NOOP
    .word @NOOP
    .word @NOOP
    .word @NOOP
    .word @EXIT
    illegal

hexchars:
    .word '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'

