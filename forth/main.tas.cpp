#include "forth_common.th"
#define TEST_LOOKUP 0

.global INBUF .global INPOS .global INLEN
.L_INBUF_before:
INBUF:
#if TEST_LOOKUP
    .utf32 "  words"
#endif
    .utf32 "                                               "
    .utf32 "                                               "
.L_INBUF_after:
INPOS: .word 0
INLEN: .word .L_INBUF_after - .L_INBUF_before

.set link, @level1_link
head(start,start): .word
    @NOOP

top: .word
    #if TEST_LOOKUP
        @TIB, @TO_IN, @FETCH, @ADD,
        @BL, @WORD,
        @FIND,
        IFNOT0(found,notfound),
    #endif

        @TIB, @DUP, @TO_IN, @FETCH, @SWAP, @SUB,    // tib used
        @IN_LEN, @SWAP, @SUB,                       // tib left
        @ACCEPT,                                    // count
        @TIB, @TO_IN, @FETCH, @ADD, @ADD, @BL, @SWAP, @STOCHR, //
        @BL, @WORD,
        @FIND, // xt flag
        IFNOT0(found,notfound)

    strip_spaces: .word
        @TIB, @TO_IN, @FETCH, @ADD,
        @FETCHR, @BL, @CMP_EQ,
        IFNOT0(advance,done_stripping)
    advance: .word
        @LITERAL, 1, @CHARS, @TO_IN, @ADDMEM,
        @LITERAL, @strip_spaces, @RELOC, @SET_IP

    done_stripping: .word
        @TIB, @TO_IN, @FETCH, @ADD,
        @FETCHR, @EMIT,
        @EXIT
    find_word: .word
        @TIB, @TO_IN, @FETCH, @ADD,
        @FIND,
        IFNOT0(found,notfound)
    notfound: .word
        // complain
        @LITERAL, @undefined_word, @RELOC, @PUTS,
        @CR,
        @ABORT

    found: .word // tib xt
        @RELOC, @EXECUTE,

        // example of a computed branch
        @LITERAL, 1,
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

undefined_word:
.L_undefined_word:
    .word (.L_undefined_word_end - .L_undefined_word - 1)
    .utf32 "undefined word"
.L_undefined_word_end:

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
    @SWAP                   // string count

L_PUTS_top: .word
    @DUP,                   // string count count
    @EQZ,                   // string count flag
    IFNOT0(L_PUTS_done,L_PUTS_emit)
L_PUTS_emit: .word
    @SWAP,                  // count string
    @DUP,                   // count string string
    @FETCHR,                // count string char
    @EMIT,                  // count string
    @ADD_1CHAR,             // count STRING
    @SWAP,                  // string count
    @SUB_1,                 // string COUNT
    GOTO(L_PUTS_top)

L_PUTS_done: .word
    @TWO_DROP,              // --
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

head(ONE,1): .word
    @ENTER,
    @LITERAL, 1,
    @EXIT

.global level2_link
.set level2_link, @link

.global dict
.set dict, @level2_link

