#include "forth_common.th"

searchterm: .utf32 "over" ; .word 0

.set link, 0
head(start,start):
interp(start): .word // top level word has no @ENTER
    @WORDS,
    @CR

top: .word
    @LIT,
    @searchterm,
    @TICK,
    @EMIT_UNSIGNED,
    @BL,
    @EMIT,
    @LIT, ':',
    @EMIT,
    @BL,
    @EMIT,
    @CR,

    // example of a computed branch
    @LIT, 0,
    IFNOT0(top,bottom)
bottom: .word

    @EXIT

head(MASK4BITS,MASK4BITS):
interp(MASK4BITS): .word @ENTER,
    @LIT, 15, @AND,
    @EXIT

head(TOHEXCHAR,>HEXCHAR):
interp(TOHEXCHAR): .word @ENTER,
    // TODO stop referring to @hexchars directly
    @LIT, @hexchars, @ADD, @FETCHR,
    @EXIT

head(PUTS,PUTS):
interp(PUTS): .word @ENTER,
    // TODO
    @EXIT

head(SET_IP,SET_IP):
interp(SET_IP): .word . + 1
    PSP <- PSP + 1
    IP  <- [PSP]
    IP  <- IP + BAS
    goto(NEXT)

hexchars:
    .word '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'

