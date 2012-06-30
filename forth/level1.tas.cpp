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
// >NUMBER  ud adr u -- ud' adr' u'
//                          convert string to number
// 2DROP  x1 x2 --                      drop 2 cells
// 2DUP   x1 x2 -- x1 x2 x1 x2       dup top 2 cells
// 2OVER  x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2  per diag
// 2SWAP  x1 x2 x3 x4 -- x3 x4 x1 x2     per diagram
// 2!     x1 x2 a-addr --              store 2 cells
// 2@     a-addr -- x1 x2              fetch 2 cells
// ABORT  i*x --   R: j*x --      clear stack & QUIT
// ABORT" i*x 0  -- i*x   R: j*x -- j*x  print msg &
//        i*x x1 --       R: j*x --      abort,x1<>0
// ABS    n1 -- +n2                   absolute value
// ACCEPT c-addr +n -- +n'    get line from terminal
// ALIGN  --                              align HERE
// ALIGNED addr -- a-addr           align given addr
// ALLOT  n --              allocate n bytes in dict
// BASE   -- a-addr           holds conversion radix
// BEGIN  -- adrs         target for backward branch
// BL     -- char                     an ASCII space
// C,     char --                append char to dict
// CELLS  n1 -- n2                 cells->adrs units
// CELL+  a-addr1 -- a-addr2   add cell size to adrs
// CHAR   -- char              parse ASCII character
// CHARS  n1 -- n2                 chars->adrs units
head(CHARS,CHARS):
    .word @ENTER
    // no-op ; chars are address units in tenyr
    .word @EXIT

// CHAR+  c-addr1 -- c-addr2   add char size to adrs
// COUNT  c-addr1 -- c-addr2 u      counted->adr/len
// CR     --                          output newline
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
// FM/MOD d1 n1 -- n2 n3     floored signed division
// HERE   -- addr         returns dictionary pointer
// HOLD   char --          add char to output string
// IF     -- adrs         conditional forward branch
// IMMEDIATE   --          make last def'n immediate
// LEAVE  --    L: -- adrs             exit DO..LOOP
// LITERAL x --      append numeric literal to dict.
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
// SPACES n --                       output n spaces
// STATE  -- a-addr             holds compiler state
// S"     --                  compile in-line string
// ."     --                 compile string to print
// S>D    n -- d          single -> double precision
// THEN   adrs --             resolve forward branch
// TYPE   c-addr +n --         type line to terminal
// UNTIL  adrs --        conditional backward branch
// U.     u --                    display u unsigned
// .      n --                      display n signed
// WHILE  -- adrs              branch for WHILE loop
// WORD   char -- c-addr n  parse word delim by char
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
// AGAIN  adrs --           uncond'l backward branch
// COMPILE,  xt --            append execution token
// DABS   d1 -- +d2        absolute value, dbl.prec.
// DNEGATE d1 -- d2         negate, double precision
// HEX    --                  set number base to hex
// PAD    -- a-addr                  user PAD buffer
// TIB    -- a-addr            Terminal Input Buffer
// WITHIN n1|u1 n2|u2 n3|u3 -- f     test n2<=n1<n3?
// WORDS  --                 list all words in dict.
head(WORDS,WORDS):
    .word . + 1
    T1   <- @level1_link    // already relocated
L_WORDS_top:
    T0   <- T1 + BAS    // T0 <- addr of next link
    T3   <- T0 + 1      // T3 <- addr of name string

L_char_top:
    T4   <- [T3]        // T4 <- character
    T5   <- T4 == 0     // T5 <- end of string ?

    T6   <- BAS - p + (@L_char_bottom - 3)
    T6   <- T6 & T5
    p    <- p + T6 + 1

    T4   -> SERIAL      // emit character
    T3   <- T3 + 1      // increment char addr
    p    <- reloc(L_char_top)
L_char_bottom:
    T4   <- 0xa         // newline
    T4   -> SERIAL

    T1   <- [T0]
    T2   <- T1 <> 0     // T2 <- continue ?
    T3   <- BAS - p + (@L_WORDS_top - 3)
    T3   <- T3 & T2
    p    <- p + T3 + 1

    goto(NEXT)

//
// extensions (possibly borrowed from CamelForth)
// ?NUMBER  c-addr -- n -1    convert string->number
//                 -- c-addr 0      if convert error
head(ISNUMBER,?NUMBER):
    .word @ENTER
    .word @LIT
    .word '0'       // load '0' onto stack

    .word @FETCHR   // fetch character from string

    .word @LIT
    .word 1
    .word @CHARS
    .word @ADD_1    // increment character pointer

    .word @CMP_LT

    .word @LIT
    .word '9'
    // TODO
    .word @EXIT

.global level1_link
.set level1_link, @link

