#include "forth_common.th"
#define TEST_LOOKUP 0

.global INBUF .global INPOS .global INLEN
.L_INBUF_before:
INBUF:
    .utf32 "                                               "
    .utf32 "                                               "
.L_INBUF_after:
INPOS: .word 0
INLEN: .word .L_INBUF_after - .L_INBUF_before

.set link, @level1_link
head(start,start): .word
    @NOOP

top: .word
        @TIB, @DUP, @TO_IN, @FETCH, @SUB,    // tib -used
        @IN_LEN, @ADD,                       // tib left
        @ACCEPT,                             // count
        @BL, @TIB, @TO_IN, @FETCH, @ADD, @ADD, @STOCHR, //
        @BL, @WORD,
        @FIND, // xt flag
        IFNOT0(found,notfound)
    found: .word // tib xt
        @RELOC, @EXECUTE,
        GOTO(top)
    notfound: .word
        @ISNUMBER,
        IFNOT0(top,undefined)   // eat flag and loop if number
    undefined: .word
        // TODO say what text we were trying to parse
        @LITERAL, @undefined_word, @RELOC, @PUTS,
        @CR,
        @ABORT

head(IN_LEN,IN_LEN): .word
    @ENTER,
    @LITERAL, @INLEN, @RELOC, @FETCH,
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
    .utf32 "0123456789ABCDEF"

head(TOHEXCHAR,>HEXCHAR): .word
    @ENTER,
    @HEXTABLE, @ADD, @FETCHR,
    @EXIT

head(PUTS,PUTS): .word // ( c-addr -- )
    @ENTER,
    @DUP,                   // c-addr c-addr
    @FETCHR,                // c-addr count
    @SWAP,                  // count c-addr
    @ADD_1CHAR,             // count string
    @OVER                   // count string count

L_PUTS_top: .word
    @EQZ,                   // count string flag
    IFNOT0(L_PUTS_done,L_PUTS_emit)
L_PUTS_emit: .word
    @DUP,                   // count string string
    @FETCHR,                // count string char
    @EMIT,                  // count string
    @ADD_1CHAR,             // count STRING
    @SWAP,                  // string count
    @SUB_1,                 // string COUNT
    @TUCK,                  // count string count
    GOTO(L_PUTS_top)

L_PUTS_done: .word
    @TWO_DROP, @DROP,       // --
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

head(SAYTOP,SAYTOP): .word
    @ENTER,
    @DUP, @EMIT_UNSIGNED, @CR,
    @EXIT

.global level2_link
.set level2_link, @link

.global dict
.set dict, @level2_link

