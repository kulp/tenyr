#include "forth_common.th"
#define TEST_LOOKUP 0

.set link, @level1_link

.global INBUF ; .global INPOS ; .global INLEN
.L_INBUF_before:
INBUF:
    .utf32 "                                               "
    .utf32 "                                               "
.L_INBUF_after:
INPOS: .word 0
INLEN: .word (.L_INBUF_after - .L_INBUF_before)

head(QUIT,QUIT): .word
        // no @ENTER for QUIT since it resets RSP anyway
        @RESET_RSP,
        @LITERAL, 0, @SOURCE_ID, @STORE

    L_QUIT_line: .word
        @LITERAL, 0, @CLEAR_TIB,            // ensure tib is empty
        @TIB, @DUP, @TO_IN, @FETCH, @SUB,   // tib -used
        @IN_LEN, @ADD,                      // tib left
        @ACCEPT,                            // acount
        @DUP, @EQZ, IFNOT0(L_QUIT_done,L_QUIT_accepted)
    L_QUIT_accepted: .word
        // write spaces to rest of TIB ; should this be required ?
        @CLEAR_TIB
    L_QUIT_parse: .word
        @BL, @WORD,                         // c-addr
        @DUP, @FETCH, @EQZ, IFNOT0(L_QUIT_line,L_QUIT_handle)
    L_QUIT_handle: .word
        @DUP, @FETCH, @TO_IN, @ADDMEM,      // update >IN
        @FIND, // xt flag
        // TODO support flag == 1 for immediate words
        IFNOT0(L_QUIT_found,L_QUIT_notfound)
    L_QUIT_notfound: .word
        @ISNUMBER,
        IFNOT0(L_QUIT_parse,L_QUIT_undefined) // eat flag and loop if number
    L_QUIT_undefined: .word
        // TODO say what text we were trying to parse
        @LITERAL, @undefined_word, @RELOC, @PUTS,
        @CR
    L_QUIT_done: .word
        @ABORT
    L_QUIT_found: .word // tib xt
        @RELOC, @EXECUTE,
        GOTO(L_QUIT_parse)

// points one past end of TIB
head(END_OF_TIB,END-OF-TIB): .word  // ( -- end-of-tib )
    @ENTER,
    @IN_LEN, @TIB, @ADD,
    @EXIT

head(PARSE_START,PARSE-START): .word // ( -- parse-start )
    @ENTER,
    @TO_IN, @FETCH, @TIB, @ADD,
    @EXIT

head(CLEAR_TIB,CLEAR-TIB): .word    // ( accept-count -- )
    // TIB    TO_IN          acount       IN_LEN
    // |------^--------------^------------|
    // |                     ^^^^^^^^^^^^^ clear this
    @ENTER,                     // acount
    @PARSE_START, @ADD,         // aoffset
    @END_OF_TIB, @OVER, @SUB,   // aoffset len
    @BL, @FILL,
    @EXIT

head(IN_LEN,IN-LEN): .word
    @ENTER,
    @LITERAL, @INLEN, @RELOC, @FETCH,
    @EXIT

head(MASK4BITS,MASK4BITS): .word
    @ENTER,
    @LITERAL, 0xf, @AND,
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

head(PUTSN,PUTSN): .word // ( string count -- )
    @ENTER,
    //@GET_PSP, @SAYTOP, @DROP,
    @TUCK                   // count string count

L_PUTSN_top: .word
    @EQZ,                   // count string flag
    IFNOT0(L_PUTSN_done,L_PUTSN_emit)
L_PUTSN_emit: .word
    @DUP,                   // count string string
    @FETCHR,                // count string char
    @EMIT,                  // count string
    @ADD_1CHAR,             // count STRING
    @SWAP,                  // string count
    @SUB_1,                 // string COUNT
    @TUCK,                  // count string count
    GOTO(L_PUTSN_top)

L_PUTSN_done: .word
    @TWO_DROP,              // --
    //@GET_PSP, @SAYTOP, @DROP,
    @EXIT

head(PUTS,PUTS): .word // ( c-addr -- )
    @ENTER,
    @GET_PSP, @SAYTOP, @DROP,
    @DUP,                   // c-addr c-addr
    @FETCHR,                // c-addr count
    @SWAP,                  // count c-addr
    @ADD_1CHAR,             // count string
    @SWAP,                  // string count
    @PUTSN,
    @GET_PSP, @SAYTOP, @DROP,
    @EXIT

head(SET_IP,SET-IP): .word (. + 1)
    S   <- S + 1
    I   <- [S]
    goto(NEXT)

head(GET_IP,GET-IP): .word (. + 1)
    I   -> [S]
    S   <- S - 1
    goto(NEXT)

head(GET_PSP,GET-PSP): .word (. + 1)
    S   -> [S]
    S   <- S - 1
    goto(NEXT)

head(GET_RSP,GET-RSP): .word (. + 1)
    R   -> [S]
    S   <- S - 1
    goto(NEXT)

head(RELOC,RELOC): .word (. + 1)
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

