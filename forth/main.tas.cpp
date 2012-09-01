#include "forth_common.th"

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
    @FIND,
    IFNOT0(found,notfound)
notfound: .word
    @ABORT

found: .word
    //@LITERAL, 0, @ACCEPT,
    //@DROP, // drop flag for now, assume found
    @DUP,         @EMIT_UNSIGNED, @BL, @EMIT, @LITERAL, ':', @EMIT, @BL, @EMIT, @CR,
    @DUP, @RELOC, @EMIT_UNSIGNED, @BL, @EMIT, @LITERAL, ':', @EMIT, @BL, @EMIT, @CR,
    @RELOC, @EXECUTE,
	//@EXIT,

    // example of a computed branch
    @LITERAL, 0,
    IFNOT0(top,bottom)
bottom: .word
    @NOOP

wordstart: .word
    @KEY,
    @DUP, @LITERAL, '\n', @CMP_EQ,
    IFNOT0(linedone,checkspace)
checkspace: .word
    @DUP, @BL, @CMP_EQ,
    IFNOT0(worddone,regular),
    @NOOP
regular: .word
    @EMIT,
    @LITERAL, @wordstart, @RELOC, @SET_IP
worddone: .word
    @DROP,
    @CR,
    @LITERAL, @wordstart, @RELOC, @SET_IP
linedone: .word
    @CR,
    @EXIT

// TODO explicit echoing
head(ACCEPT,ACCEPT): .word
    @ENTER

L_ACCEPT_top: .word
    @DUP,
    //IFNOT0(L_ACCEPT_get_one,L_ACCEPT_done)
    @NOOP
L_ACCEPT_done: .word
    @LITERAL, 'B', @EMIT, @BL, @EMIT,
    @ABORT

L_ACCEPT_get_one: .word
    @EXIT

head(MASK4BITS,MASK4BITS): .word
    @ENTER,
    @LITERAL, 15, @AND,
    @EXIT

head(HEXTABLE,HEXTABLE): .word
    @ENTER,
    @LITERAL, @hexchars, @RELOC,
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
    S   <- S + 1
    I   <- [S]
    goto(NEXT)

head(GET_IP,GET_IP): .word . + 1
    I   -> [S]
    S   <- S - 1
    goto(NEXT)

head(GET_PSP,GET_PSP): .word . + 1
    S   -> [S]
    S   <- S - 1
    goto(NEXT)

head(GET_RSP,GET_RSP): .word . + 1
    R   -> [S]
    S   <- S - 1
    goto(NEXT)

head(RELOC,RELOC): .word . + 1
    W   <- [S + 1]
    W   <- rel(W)
    W   -> [S + 1]
    goto(NEXT)

