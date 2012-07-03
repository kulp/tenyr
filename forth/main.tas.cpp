#include "forth_common.th"

searchterm: .utf32 "over" ; .word 0

.set link, 0
head(start,start): .word // top level word has no @ENTER
    @WORDS,
    @CR,

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

    @EXIT

head(MASK4BITS,MASK4BITS): .word @ENTER,
    @LIT, 15, @AND,
    @EXIT

head(TOHEXCHAR,>HEXCHAR): .word @ENTER,
    // TODO stop referring to @hexchars directly
    @LIT, @hexchars, @ADD, @FETCHR,
    @EXIT

head(PUTS,PUTS): .word @ENTER,
    // TODO
    @EXIT

hexchars:
    .word '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'

