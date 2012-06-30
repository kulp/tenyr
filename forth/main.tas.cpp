#include "forth_common.th"

.set link, 0
head(start,start):
    .word @FLOOP
    .word @WORDS
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
    .word @LIT
    .word 65
    .word @DUP
    .word @EMIT
    .word @LIT
    .word 32
    .word @ADD
    .word @EMIT
    .word @LIT
    .word 10
    .word @EMIT
    .word @NOOP
    .word @NOOP
    .word @NOOP
    .word @NOOP
    .word @EXIT
    illegal

