#include "forth_common.th"

// A little Forth.

.set link, 0
.global __zero
__zero:
__boot:
    S <- [reloc(_PSPinit)]
    R <- [reloc(_RSPinit)]
    I <- reloc(QUIT)        // enter text interpreter
    goto(NEXT)

    .global _PSPinit
    .global _RSPinit
_PSPinit:   .word   0x007fffff
_RSPinit:   .word   0x00ffffff

// TODO smudge internal words
head(RESET_RSP,RESET_RSP): .word . + 1
    R <- [reloc(_RSPinit)]
    goto(NEXT)

head(NEXT,NEXT):
    W  <- [I]
    W  <-  rel(W)
    I  <-  I + 1
    X  <- [W]
    jmp(X)

head(ENTER,ENTER):
    .word . + 1
    push(R,I)
    I  <- W + 1
    goto(NEXT)

head(EXIT,EXIT):
    .word . + 1
    pop(R,I)
    goto(NEXT)

    .global source_id
source_id: .word 0
head(SOURCE_ID,SOURCE-ID): .word
    @ENTER, @LITERAL, @source_id, @RELOC, @EXIT

_done:
    .word @ABORT

