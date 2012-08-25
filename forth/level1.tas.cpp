#include "forth_common.th"
#include "serial.th"

.set link, @level0_link

// These comments were adapted from CamelForth's
// glosshi.txt
// NAME   stack in -- stack out          description
//
//   Guide to stack diagrams:  R: = return stack,
//   c = 32-bit character, flag = boolean (0 or -1),
//   n = signed 32-bit, u = unsigned 32-bit,
//   d = signed 64-bit, ud = unsigned 64-bit,
//   +n = unsigned 31-bit, x = any cell value,
//   i*x j*x = any number of cell values,
//   a-addr = aligned adrs, c-addr = character adrs
//   p-addr = I/O port adrs, sys = system-specific.
//   Refer to ANS Forth document for more details.
//
//                ANS Forth Core words
// These are required words whose definitions are
// specified by the ANS Forth document.
//
// #      ud1 -- ud2       convert 1 digit of output
// #S     ud1 -- ud2        convert remaining digits
// #>     ud1 -- c-addr u      end conv., get string
// '      -- xt              find word in dictionary
// (      --                      skip input until )
// *      n1 n2 -- n3                signed multiply
// */     n1 n2 n3 -- n4                    n1*n2/n3
// */MOD  n1 n2 n3 -- n4 n5     n1*n2/n3, rem & quot
// +LOOP  adrs --   L: 0 a1 a2 .. aN --
// ,      x --                   append cell to dict
// /      n1 n2 -- n3                  signed divide
// /MOD   n1 n2 -- n3 n4   signed divide, rem & quot
// :      --                begin a colon definition
// ;                          end a colon definition
// <#     --                begin numeric conversion
// >BODY  xt -- a-addr           adrs of param field
// >IN    -- a-addr            holds offset into TIB
head(TO_IN,>IN): .word
    @ENTER,
    @LITERAL, @INPOS, @RELOC,
    @EXIT

// >NUMBER  ud adr u -- ud' adr' u'
//                          convert string to number
// 2DROP  x1 x2 --                      drop 2 cells
head(TWO_DROP,2DROP): .word
    @ENTER,
    @DROP, @DROP,
    @EXIT

// 2DUP   x1 x2 -- x1 x2 x1 x2       dup top 2 cells
head(TWO_DUP,2DUP): .word
    @ENTER,         // a b
    @OVER, @OVER,   // a b a b
    @EXIT

// 2OVER  x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2  per diag
head(TWO_OVER,2OVER): .word
    @ENTER,             // a b c d
    @TWO_SWAP,          // c d a b
    @TWO_DUP,           // c d a b a b
    @PUSH_R, @PUSH_R,   // c d a b  R: -- b a
    @TWO_SWAP,          // a b c d
    @POP_R, @POP_R,     // a b c d a b
    @EXIT

// 2SWAP  x1 x2 x3 x4 -- x3 x4 x1 x2     per diagram
head(TWO_SWAP,2SWAP): .word
    @ENTER,         // a b c d --
    @PUSH_R,        // a b c    R: -- d
    @ROT, @ROT,     // c a b
    @POP_R,         // c a b d  R: --
    @ROT, @ROT,     // c d a b
    @EXIT

// 2!     x1 x2 a-addr --              store 2 cells
// 2@     a-addr -- x1 x2              fetch 2 cells
// ABORT  i*x --   R: j*x --      clear stack & QUIT
head(ABORT,ABORT):
    .word . + 1
    S   <- [reloc(_PSPinit)]
    R   <- [reloc(_RSPinit)]
    illegal

// ABORT" i*x 0  -- i*x   R: j*x -- j*x  print msg &
//        i*x x1 --       R: j*x --      abort,x1<>0
// ABS    n1 -- +n2                   absolute value
// ACCEPT c-addr +n -- +n'    get line from terminal
head(ACCEPT,ACCEPT): .word                      // ( c-addr +n1 -- +n2 )
// TODO explicit echoing
    @ENTER,

    @OVER, @SWAP                                // C-addr c-addr n1
L_ACCEPT_key: .word
    @KEY,                                       // C-addr c-addr n1 c
    @DUP, @LITERAL, '\n', @CMP_EQ,              // C-addr c-addr n1 c flag
    IFNOT0(L_ACCEPT_exit,L_ACCEPT_regular)
L_ACCEPT_regular: .word
    @ROT,                                       // C-addr n1 c c-addr
    @TUCK,                                      // C-addr n1 c-addr c c-addr
    @STOCHR,                                    // C-addr n1 c-addr
    @LITERAL, 1, @CHARS, @ADD,                  // C-addr n1 c-addr++
    @SWAP,                                      // C-addr c-addr n1
    @SUB_1,                                     // C-addr c-addr n1--
    @DUP, @EQZ,                                 // C-addr c-addr n1 flag
    IFNOT0(L_ACCEPT_done,L_ACCEPT_key)
L_ACCEPT_exit: .word
    @DROP                                       // C-addr c-addr n1
L_ACCEPT_done: .word
    @DROP,                                      // C-addr c-addr
    @SWAP, @SUB,                                // n2
    @EXIT

// ALIGN  --                              align HERE
// ALIGNED addr -- a-addr           align given addr
// ALLOT  n --              allocate n bytes in dict
// BASE   -- a-addr           holds conversion radix
// BEGIN  -- adrs         target for backward branch
// BL     -- char                     an ASCII space
head(BL,BL): .word
    @ENTER,
    @LITERAL, ' ',
    @EXIT

// C,     char --                append char to dict
// CELLS  n1 -- n2                 cells->adrs units
head(CELLS,CELLS): .word
    @ENTER,
    // no-op ; cells are address units in tenyr
    @EXIT

// CELL+  a-addr1 -- a-addr2   add cell size to adrs
// CHAR   -- char              parse ASCII character
// CHARS  n1 -- n2                 chars->adrs units
head(CHARS,CHARS): .word
    @ENTER,
    // no-op ; chars are address units in tenyr
    @EXIT

// CHAR+  c-addr1 -- c-addr2   add char size to adrs
head(CHAR_PLUS,CHAR+): .word
    @ADD_1,
    @EXIT

// COUNT  c-addr1 -- c-addr2 u      counted->adr/len
// CR     --                          output newline
head(CR,CR): .word
    @ENTER,
    @LITERAL, '\n', @EMIT,
    @EXIT

// CREATE --              create an empty definition
// DECIMAL --             set number base to decimal
// DEPTH  -- +n             number of items on stack
// DO     -- adrs   L: -- 0        start of DO..LOOP
// DOES>  --           change action of latest def'n
// ELSE   adrs1 -- adrs2         branch for IF..ELSE
// ENVIRONMENT?  c-addr u -- false      system query
// EVALUATE  i*x c-addr u -- j*x    interpret string
// FIND   c-addr -- c-addr 0     ..if name not found
//                  xt  1        ..if immediate
//                  xt -1        ..if "normal"
head(FIND,FIND):
    .word . + 1
    T0   <- @dict       // T0 <- addr of dictionary
L_FIND_top:
    T4   <- [S + 1]     // T4 <- name to look up
    T1   <- T0 + 2      // T1 <- addr of name string
    T6   <- rel(T0)     // T6 <- addr of rec length
    T6   <- [T6 - 1]    // T6 <- rec length
    T6   <- T6 - 2      // T6 <- test-name length
    T2   <- [T4]        // T2 <- find-name length
    T2   <- T6 <> T2    // check length match
    iftrue(T2,L_FIND_char_bottom)
    T4   <- T4 + 1      // T4 <- addr of test string

L_FIND_char_top:
    T2   <- [rel(T1)]   // T2 <- test-name char
    T3   <- [T4]        // T3 <- find-name char

    // uppercase test-name char
    T2   <- T2 &~ ('a' ^ 'A')
    // uppercase find-name char
    T3   <- T3 &~ ('a' ^ 'A')

    T2   <- T2 <> T3    // T2 <- char mismatch ?
    T3   <- T6 <  1     // T3 <- end of test-name ?

    iftrue(T3,L_FIND_match)
    iftrue(T2,L_FIND_char_bottom)

    T1   <- T1 + 1      // increment test-name addr
    T6   <- T6 - 1      // decrement test-name length
    T4   <- T4 + 1      // increment find-name addr
    P    <- reloc(L_FIND_char_top)

L_FIND_char_bottom:
    T0   <- [rel(T0)]   // T0 <- follow link
    T1   <- T0 <> 0     // T1 <- more words ? .word . + 1
    T2   <- - P + (@L_FIND_top - 3)
    T2   <- rel(T2)
    T2   <- T2 & T1
    P    <- P + T2

    // If we reach this point, there was a mismatch.
    S    <- S - 1
    A    -> [S + 1]
    goto(NEXT)

L_FIND_match:
    S    <- S - 1
    T0   -> [S + 2]     // put xt on stack
    T0   <- -1
    // TODO support flag for immediate words
    T0   -> [S + 1]     // put flag on stack

    goto(NEXT)

// FM/MOD d1 n1 -- n2 n3     floored signed division
// HERE   -- addr         returns dictionary pointer
head(HEAD,HEAD): .word
    @ENTER,
    @LITERAL, @dict, @RELOC,
    @EXIT

// HOLD   char --          add char to output string
// IF     -- adrs         conditional forward branch
// IMMEDIATE   --          make last def'n immediate
// LEAVE  --    L: -- adrs             exit DO..LOOP
// LITERAL x --      append numeric literal to dict.
head(LITERAL,LITERAL):
    .word . + 1
    W   <- [I]
    I   <- I + 1
    W   -> [S]
    S   <- S - 1
    goto(NEXT)

// LOOP   adrs --   L: 0 a1 a2 .. aN --
// MAX    n1 n2 -- n3                 signed maximum
// MIN    n1 n2 -- n3                 signed minimum
// MOD    n1 n2 -- n3               signed remainder
// MOVE   addr1 addr2 u --                smart move
// M*     n1 n2 -- d       signed 32*32->64 multiply
// POSTPONE  --      postpone compile action of word
// QUIT   --    R: i*x --    interpret from keyboard
// RECURSE --             recurse current definition
// REPEAT adrs1 adrs2 --          resolve WHILE loop
// SIGN   n --                 add minus sign if n<0
// SM/REM d1 n1 -- n2 n3   symmetric signed division
// SOURCE -- adr n              current input buffer
// SPACE  --                          output a space
head(SPACE,SPACE): .word
    @ENTER,
    @BL, @EMIT,
    @EXIT

// SPACES n --                       output n spaces
// STATE  -- a-addr             holds compiler state
// S"     --                  compile in-line string
// ."     --                 compile string to print
// S>D    n -- d          single -> double precision
// THEN   adrs --             resolve forward branch
// TYPE   c-addr +n --         type line to terminal
// UNTIL  adrs --        conditional backward branch
// U.     u --                    display u unsigned
head(EMIT_UNSIGNED,U.): .word
    @ENTER,

    // TODO rewrite this as a loop, and use less stack
    // TODO pay attention to BASE
    @DUP, @LITERAL, 4, @RSHIFT,
    @DUP, @LITERAL, 4, @RSHIFT,
    @DUP, @LITERAL, 4, @RSHIFT,
    @DUP, @LITERAL, 4, @RSHIFT,
    @DUP, @LITERAL, 4, @RSHIFT,
    @DUP, @LITERAL, 4, @RSHIFT,
    @DUP, @LITERAL, 4, @RSHIFT,

    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,
    @MASK4BITS, @TOHEXCHAR, @EMIT,

    @EXIT

// .      n --                      display n signed
// WHILE  -- adrs              branch for WHILE loop
// WORD   char -- c-addr    parse word delim by char
head(WORD,WORD): .word
    @ENTER,                 // c
    @WORD_TMP,              // c TMP
    @LITERAL, 0, @OVER,     // c TMP 0 TMP
    @STOCHR,                // c TMP
    @ADD_1CHAR,             // c TMP+1
    @TIB, @TO_IN, @FETCH,   // c TMP TIB off
    @ADD,                   // c TMP tib
    @ROT, @SWAP, @TWO_DUP,  // TMP c tib c tib
    @LITERAL, .L_WORD_tmp_end - .L_WORD_tmp,
    @SKIP,                  // TMP c tib ntib
    @NIP,                   // TMP c ntib
    @SWAP, @ROT, @ROT,      // c TMP ntib
    @NOOP

L_WORD_top: .word
    // c TMP tib
    @ROT, @OVER,            // TMP tib c tib
    @FETCHR, @DUP, @ROT,    // TMP tib c2 c2 c1
    @TUCK,                  // TMP tib c2 c1 c2 c1
    @CMP_EQ,                // TMP tib c2 c1 flag
    IFNOT0(L_WORD_done_stripping,L_WORD_advance)

L_WORD_advance: .word
    // tmp tib c2 c1
    @SWAP,                  // TMP tib c1 c2
    @TWO_OVER, @DROP,       // TMP tib c1 c2 TMP
    @TUCK, @STOCHR,         // TMP tib c1 tmp
    @ADD_1CHAR,             // TMP tib c1 tmp+1
    @ROT,                   // TMP c1 tmp tib
    @ADD_1CHAR,             // TMP c1 tmp tib+1
    @TWO_SWAP, @NIP,        // tmp tib c1
    @WORD_TMP, @DUP,        // tmp tib c1 TMP TMP
    @FETCHR, @ADD_1,        // tmp tib c1 TMP nc+1
    @SWAP, @STOCHR,         // tmp tib c1
    @ROT, @ROT,             // c1 tmp tib
    @LITERAL, @L_WORD_top, @RELOC, @SET_IP

L_WORD_done_stripping: .word
    // tmp tib c2 c1
    @TWO_DROP, @DROP,       // tmp
    @LITERAL, @BL,          // tmp bl
    @SWAP, @STOCHR,         // 
    @WORD_TMP,              // TMP
    @EXIT
L_WORD_tmp:
.L_WORD_tmp:
    .utf32 "          ""          ""          ""  "
.L_WORD_tmp_end: .word 0

// ( char c-addr u -- c-addr )    skip init char up to N
head(SKIP,SKIP): .word
    @ENTER                  // c addr u

L_SKIP_top: .word
    // c addr u
    @DUP, @EQZ,             // c addr u flag
    IFNOT0(L_SKIP_done,L_SKIP_cont)

L_SKIP_cont: .word
    // c addr u
    @ROT, @ROT,             // u c addr
    @SWAP, @TWO_DUP,        // u addr c addr c
    @SWAP, @FETCHR,         // u addr c1 c1 c2
    @CMP_EQ,                // u addr c1 flag
    IFNOT0(L_SKIP_inc,L_SKIP_done)

L_SKIP_inc: .word
    // u addr c
    @SWAP, @ADD_1CHAR,      // u c addr+1
    @ROT,                   // c addr u
    @LITERAL, @L_SKIP_top, @RELOC, @SET_IP

L_SKIP_done: .word
    // x addr x
    @DROP, @NIP,
    @EXIT

head(ADD_1CHAR,C1+): .word
    @ENTER,
    // TODO we could just do ADD_1
    @LITERAL, 1, @CHARS, @ADD,
    @EXIT

head(WORD_TMP,WORD_TMP): .word
    @ENTER,
    @LITERAL, @L_WORD_tmp, @RELOC,
    @EXIT

// [      --                enter interpretive state
// [CHAR] --               compile character literal
// [']    --          find word & compile as literal
// ]      --                   enter compiling state
//
//                ANS Forth Extensions
// These are optional words whose definitions are
// specified by the ANS Forth document.
//
// .S     --                    print stack contents
// /STRING a u n -- a+n u-n              trim string
head(SLASH_STRING,/STRING): .word
    @ENTER,
    // TODO
    @DROP,
    @EXIT

// AGAIN  adrs --           uncond'l backward branch
// COMPILE,  xt --            append execution token
// DABS   d1 -- +d2        absolute value, dbl.prec.
// DNEGATE d1 -- d2         negate, double precision
// HEX    --                  set number base to hex
// PAD    -- a-addr                  user PAD buffer
// TIB    -- a-addr            Terminal Input Buffer
head(TIB,TIB): .word
    @ENTER,
    @LITERAL, @INBUF, @RELOC,
    @EXIT

// WITHIN n1|u1 n2|u2 n3|u3 -- f     test n2<=n1<n3?
// WORDS  --                 list all words in dict.
head(WORDS,WORDS):
    .word . + 1
    T0   <- @dict       // already relocated
L_WORDS_top:
    T0   <- rel(T0)     // T0 <- addr of next link
    T1   <- T0 + 2      // T1 <- addr of name string
    T4   <- [T0 - 1]    // T4 <- rec length
    T4   <- T4 - 2      // T4 <- test-name length

L_WORDS_char_top:
    T2   <- [T1]        // T2 <- character
    T3   <- T4 <  1     // T3 <- end of string ?

    iftrue(T3,L_WORDS_char_bottom)

    T2   -> SERIAL      // emit character
    T1   <- T1 + 1      // increment char addr
    T4   <- T4 - 1      // decrement string length
    P    <- reloc(L_WORDS_char_top)
L_WORDS_char_bottom:
    T1   <- '\n'
    T1   -> SERIAL

    T0   <- [T0]
    T1   <- T0 <> 0     // T1 <- continue ?

    iftrue(T1,L_WORDS_top)

    goto(NEXT)

//
// extensions (possibly borrowed from CamelForth)
// ?NUMBER  c-addr -- n -1    convert string->number
//                 -- c-addr 0      if convert error
head(ISNUMBER,?NUMBER): .word
    @ENTER,
    // TODO make sensitive to BASE
    @DUP,                       // c-addr c-addr
    @LITERAL, 1, @CHARS, @ADD,  // ca ca+1
    @SWAP,                  // ca+1 ca
    @FETCHR,                // ca+1 ch
    @LITERAL, ('0' - 1),    // ca+1 ch '0'-1
    @CMP_GT,                // ca+1 ch flag
    @OVER,                  // ca+1 ch flag ch
    @LITERAL, ('9' + 1),    // ca+1 ch flag ch '9'+1
    @CMP_LT,                // ca+1 ch flag flag
    @AND,                   // ca+1 ch flag

    @LITERAL, 1, @CHARS,
    @ADD_1,     // increment character pointer

    @CMP_LT,

    // TODO
    @EXIT

head(DEBUG,DEBUG): .word . + 1
    illegal
    //@ENTER,
    //@ABORT,
    //@EXIT

.global level1_link
.set level1_link, @link

.global dict
.set dict, @level1_link

