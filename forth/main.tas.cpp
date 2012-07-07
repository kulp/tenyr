#include "forth_common.th"

searchterm: .utf32 "words" ; .word 0

.global INBUF
INBUF: .utf32 "words"
.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 # 84 long
.global INPOS
INPOS: .word 0

.set link, 0
head(start,start): .word
    @NOOP

top: .word
    @TIB, @TO_IN, @FETCH, @ADD,
    //@LIT, @searchterm, @RELOC,
    @FIND,
    @DROP, // drop flag for now, assume found
    @DUP,         @EMIT_UNSIGNED, @BL, @EMIT, @LIT, ':', @EMIT, @BL, @EMIT, @CR,
    @DUP, @RELOC, @EMIT_UNSIGNED, @BL, @EMIT, @LIT, ':', @EMIT, @BL, @EMIT, @CR,
    //@GET_RSP, @EMIT_UNSIGNED, @CR,
    //@DUP, @RELOC, @EMIT_UNSIGNED, @CR,
    //@RELOC, @EXECUTE,
    //@GET_RSP, @EMIT_UNSIGNED, @CR,

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

head(MASK4BITS,MASK4BITS): .word
    @ENTER,
    @LIT, 15, @AND,
    @EXIT

head(HEXTABLE,HEXTABLE): .word
    @ENTER,
    @LIT, @hexchars, @RELOC,
    @EXIT

hexchars:
    .word '0','1','2','3','4','5','6','7',
          '8','9','A','B','C','D','E','F'

head(TOHEXCHAR,>HEXCHAR): .word
    @ENTER,
    @HEXTABLE, @ADD, @FETCHR,
    @EXIT

head(PUTS,PUTS): .word
    @ENTER,
    // TODO
    @EXIT

head(SET_IP,SET_IP): .word . + 1
    PSP <- PSP + 1
    IP  <- [PSP]
    goto(NEXT)

head(GET_IP,GET_IP): .word . + 1
    IP  -> [PSP]
    PSP <- PSP - 1
    goto(NEXT)

head(GET_PSP,GET_PSP): .word . + 1
    PSP -> [PSP]
    PSP <- PSP - 1
    goto(NEXT)

head(GET_RSP,GET_RSP): .word . + 1
    RSP -> [PSP]
    PSP <- PSP - 1
    goto(NEXT)

head(RELOC,RELOC): .word . + 1
    W <- [PSP + 1]
    W <- W + BAS
    W -> [PSP + 1]
    goto(NEXT)

