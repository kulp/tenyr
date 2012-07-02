#include "forth_common.th"

searchterm: .utf32 "OVER" ; .word 0

.set link, 0
head(start,start):
    //.word @FLOOP
    .word

    @WORDS,
    @CR,

    @LIT,
    @searchterm,
    @TICK,
    @EMIT32HEX,
    @CR

    .word @EXIT

head(EMIT32HEX,EMIT32HEX):
    .word
    @ENTER,

    // TODO rewrite this as a loop, and use less stack
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,

    // TODO stop referring to @hexchars directly
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,
    @LIT, 15, @AND, @LIT, @hexchars, @ADD, @FETCHR, @EMIT,

    @EXIT

hexchars:
    .word '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'

