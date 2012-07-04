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
    @NOOP

wordstart: .word
    @KEY,
    @DUP, @LIT, '\n', @CMP_EQ,
    IFNOT0(linedone,checkspace)
checkspace: .word
    @DUP, @BL, @CMP_EQ,
    IFNOT0(worddone,regular),
    @NOOP
regular: .word
    @EMIT,
    @LIT, @wordstart, @RELOC, @SET_IP
worddone: .word
    @DROP,
    @CR,
    @LIT, @wordstart, @RELOC, @SET_IP
linedone: .word
    @CR,
    @EXIT

head(MASK4BITS,MASK4BITS):
interp(MASK4BITS): .word @ENTER,
    @LIT, 15, @AND,
    @EXIT

head(HEXTABLE,HEXTABLE):
interp(HEXTABLE): .word @ENTER,
    @LIT, @hexchars, @RELOC,
    @EXIT

head(TOHEXCHAR,>HEXCHAR):
interp(TOHEXCHAR): .word @ENTER,
    @HEXTABLE, @ADD, @FETCHR,
    @EXIT

head(PUTS,PUTS):
interp(PUTS): .word @ENTER,
    // TODO
    @EXIT

head(SET_IP,SET_IP):
interp(SET_IP): .word . + 1
    PSP <- PSP + 1
    IP  <- [PSP]
    goto(NEXT)

head(GET_IP,GET_IP):
interp(GET_IP): .word . + 1
    IP  -> [PSP]
    PSP <- PSP - 1
    goto(NEXT)

head(GET_PSP,GET_PSP):
interp(GET_PSP): .word . + 1
    PSP -> [PSP]
    PSP <- PSP - 1
    goto(NEXT)

head(GET_RSP,GET_RSP):
interp(GET_RSP): .word . + 1
    RSP -> [PSP]
    PSP <- PSP - 1
    goto(NEXT)

head(RELOC,RELOC):
interp(RELOC): .word . + 1
    W <- [PSP + 1]
    W <- W + BAS
    W -> [PSP + 1]
    goto(NEXT)

hexchars:
    .word '0','1','2','3','4','5','6','7',
          '8','9','A','B','C','D','E','F'

