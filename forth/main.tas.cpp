#include "forth_common.th"

searchterm: .utf32 "?NUMBER" ; .word 0

.set link, 0
head(start,start):
    //.word @FLOOP
    .word @WORDS
    .word @LIT
    .word @searchterm
    .word @TICK

    .word @DUP
    .word @LIT, 4
    .word @RSHIFT

    .word @DUP
    .word @LIT, 4
    .word @RSHIFT

    .word @DUP
    .word @LIT, 4
    .word @RSHIFT

    .word @LIT, 15
    .word @AND
    //.word @LIT, 'A'
    .word @LIT, @hexes
    .word @ADD
    .word @FETCHR
    .word @EMIT

    .word @LIT, 15
    .word @AND
    //.word @LIT, 'A'
    //.word @ADD
    .word @EMIT

    .word @LIT, 15
    .word @AND
    //.word @LIT, 'A'
    //.word @ADD
    .word @EMIT

    .word @LIT, 15
    .word @AND
    //.word @LIT, 'A'
    //.word @ADD
    .word @EMIT
    .word @CR

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

hexes:
    .word '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'

