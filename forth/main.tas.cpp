#include "forth_common.th"

searchterm: .utf32 "OVER" ; .word 0

.set link, 0
head(start,start): .word // top level word has no @ENTER
    @WORDS,
    @CR,

    @LIT,
    @searchterm,
    @TICK,
    @EMIT32HEX,
    @CR,

    @EXIT

head(EMIT32HEX,EMIT32HEX): .word @ENTER,

    // TODO rewrite this as a loop, and use less stack
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,
    @DUP, @LIT, 4, @RSHIFT,

    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,

    @EXIT

head(MASK4BITS,MASK4BITS): .word @ENTER,
    @LIT, 15, @AND,
    @EXIT

head(TOHEXCHAR,>HEXCHAR): .word @ENTER,
    // TODO stop referring to @hexchars directly
    @LIT, @hexchars, @ADD, @FETCHR,
    @EXIT

hexchars:
    .word '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'

