#include "forth_common.th"

// A little Forth kernel.
// Expects a global symbol called `start' to be defined in another object, which
// should be a list of addresses of code to run, ending with EXIT.

__boot:
    BAS <- p - .
    PSP <- [reloc(_PSPinit)]
    RSP <- [reloc(_RSPinit)]
    push(RSP,reloc(_done))
    IP  <- reloc(start)
    goto(NEXT)

    .global _PSPinit
    .global _RSPinit
_PSPinit:   .word   0x007fffff
_RSPinit:   .word   0x00ffffff

    .global NEXT
NEXT:
    W  <- [IP]
    W  <-  W + BAS
    IP <-  IP + 1
    X  <- [W]
    jmp(X)

head(ENTER,ENTER):
    .word . + 1
    push(RSP,IP)
    IP <- W + 1
    goto(NEXT)

_done:
    .word @BYE

