#include "forth_common.th"
#include "serial.th"

#define BINOP(Op)    \
    .word . + 1      \
    T0  <- [PSP + 2] \
    T1  <- [PSP + 1] \
    W   <-  T0 Op T1 \
    PSP <-  PSP + 1  \
    W   -> [PSP + 1] \
    goto(NEXT)

.set link, 0

// These comments were adapted from CamelForth
// http://www.bradrodriguez.com/papers/glosslo.txt
//
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
// !      x a-addr --           store cell in memory
head(STORE,!):
    .word . + 1
    T0  <- [PSP + 2]
    T1  <- [PSP + 1]
    PSP <-  PSP + 2
    T0  -> [T1]
    goto(NEXT)

// +      n1/u1 n2/u2 -- n3/u3             add n1+n2
head(ADD,+): BINOP(+)

// +!     n/u a-addr --           add cell to memory
head(ADDMEM,+!):
    .word . + 1
    T0  <- [PSP + 2]
    T1  <- [PSP + 1]
    PSP <-  PSP + 2
    T2  <- [T1]
    T2  <- T2 + T0
    T2  -> [T1]
    goto(NEXT)

// -      n1/u1 n2/u2 -- n3/u3        subtract n1-n2
head(SUB,-): BINOP(-)

// <      n1 n2 -- flag           test n1<n2, signed
head(CMP_LT,<):
    .word . + 1
    T0  <- [PSP + 2]
    T1  <- [PSP + 1]
    PSP <- PSP + 1
    T2  <- T0 < T1
    T2  -> [PSP + 1]
    goto(NEXT)

// =      x1 x2 -- flag                   test x1=x2
head(CMP_EQ,=): BINOP(==)

// >      n1 n2 -- flag           test n1>n2, signed
head(CMP_GT,>): BINOP(>)

// >R     x --   R: -- x        push to return stack
head(PUSH_R,>R):
    .word . + 1
    W   <- [PSP + 1]
    W   -> [RSP]
    RSP <-  RSP - 1
    PSP <-  PSP + 1
    goto(NEXT)

// ?DUP   x -- 0 | x x                DUP if nonzero
head(DUPNZ,?DUP):
    .word . + 1
    T0  <- [PSP + 1]
    T1  <- T0 <> 0
    PSP <-  PSP + T1
    T0  -> [PSP + 1]
    goto(NEXT)

// @      a-addr -- x         fetch cell from memory
head(FETCH,@):
    .word . + 1
    W <- [PSP + 1]
    W <- [W]
    W -> [PSP + 1]
    goto(NEXT)

// 0<     n -- flag             true if TOS negative
head(LTZ,0<):
    .word . + 1
    T0  <- [PSP + 1]
    T0  <- T0 < 0
    T0  -> [PSP + 1]
    goto(NEXT)

// 0=     n/u -- flag           return true if TOS=0
head(EQZ,0=):
    .word . + 1
    W <- [PSP + 1]
    W <- W == 0
    W -> [PSP + 1]
    goto(NEXT)

// 1+     n1/u1 -- n2/u2                add 1 to TOS
head(ADD_1,1+):
    .word . + 1
    W <- [PSP + 1]
    W <- W + 1
    W -> [PSP + 1]
    goto(NEXT)

// 1-     n1/u1 -- n2/u2         subtract 1 from TOS
head(SUB_1,1-):
    .word . + 1
    W <- [PSP + 1]
    W <- W - 1
    W -> [PSP + 1]
    goto(NEXT)

// 2*     x1 -- x2             arithmetic left shift
head(MUL_2,2*):
    .word . + 1
    W <- [PSP + 1]
    W <- W << 1
    W -> [PSP + 1]
    goto(NEXT)

// 2/     x1 -- x2            arithmetic right shift
head(DIV_2,2/):
    .word . + 1
    W <- [PSP + 1]
    W <- W >> 1
    W -> [PSP + 1]
    goto(NEXT)

// AND    x1 x2 -- x3                    logical AND
head(AND,AND): BINOP(&)

// CONSTANT   n --           define a Forth constant
// C!     c c-addr --           store char in memory
head(STOCHR,C!):
    .word @ENTER
    .word @STORE
    .word @EXIT

// C@     c-addr -- c         fetch char from memory
head(FETCHR,C@):
    .word @ENTER
    .word @FETCH
    .word @EXIT

// DROP   x --                     drop top of stack
head(DROP,DROP):
    .word . + 1
    PSP <- PSP + 1
    goto(NEXT)

// DUP    x -- x x            duplicate top of stack
head(DUP,DUP):
    .word . + 1
    W   <- [PSP + 1]
    PSP <- PSP - 1
    W   -> [PSP + 1]
    goto(NEXT)

// EMIT   c --           output character to console
head(EMIT,EMIT):
    .word . + 1
    W   <- [PSP + 1]
    PSP <- PSP + 1
    W   -> SERIAL
    goto(NEXT)

// EXECUTE   i*x xt -- j*x   execute Forth word 'xt'
// EXIT   --                 exit a colon definition
head(EXIT,EXIT):
    .word . + 1
    pop(RSP,IP)
    goto(NEXT)

// FILL   c-addr u c --        fill memory with char
// I      -- n   R: sys1 sys2 -- sys1 sys2
//                      get the innermost loop index
// INVERT x1 -- x2                 bitwise inversion
head(INVERT,INVERT):
    .word . + 1
    W <- [PSP + 1]
    W <- W ^ -1
    W -> [PSP + 1]
    goto(NEXT)

// J      -- n   R: 4*sys -- 4*sys
//                         get the second loop index
// KEY    -- c           get character from keyboard
head(KEY,KEY):
    .word . + 1
    W   <- SERIAL
    PSP <- PSP - 1
    W   -> [PSP + 1]
    goto(NEXT)

// LSHIFT x1 u -- x2        logical L shift u places
head(LSHIFT,LSHIFT): BINOP(<<)

// NEGATE x1 -- x2                  two's complement
head(NEGATE,NEGATE):
    .word . + 1
    W <- [PSP + 1]
    W <- A - W
    W -> [PSP + 1]
    goto(NEXT)

// OR     x1 x2 -- x3                     logical OR
head(OR,OR): BINOP(|)

// OVER   x1 x2 -- x1 x2 x1        per stack diagram
head(OVER,OVER):
    .word . + 1
    PSP <-  PSP - 1
    W   <- [PSP + 3]
    W   -> [PSP + 1]
    goto(NEXT)

// ROT    x1 x2 x3 -- x2 x3 x1     per stack diagram
head(ROT,ROT):
    .word . + 1
    T0  <- [PSP + 3]
    T1  <- [PSP + 2]
    T2  <- [PSP + 1]

    T1  -> [PSP + 3]
    T2  -> [PSP + 2]
    T0  -> [PSP + 1]
    goto(NEXT)

// RSHIFT x1 u -- x2        logical R shift u places
// R>     -- x    R: x --      pop from return stack
head(POP_R,R>):
    .word . + 1
    W   <- [RSP + 1]
    RSP <-  RSP + 1
    PSP <-  PSP - 1
    W   -> [PSP + 1]
    goto(NEXT)

// R@     -- x    R: x -- x       fetch from rtn stk
head(FETCH_R,R@):
    .word . + 1
    W   <- [RSP + 1]
    PSP <-  PSP - 1
    W   -> [PSP + 1]
    goto(NEXT)

// SWAP   x1 x2 -- x2 x1          swap top two items
head(SWAP,SWAP):
    .word . + 1
    T0  <- [PSP + 2]
    T1  <- [PSP + 1]

    T1  -> [PSP + 2]
    T0  -> [PSP + 1]
    goto(NEXT)

// UM*    u1 u2 -- ud       unsigned 32x32->64 mult.
// UM/MOD ud u1 -- u2 u3     unsigned 64/32->32 div.
// UNLOOP --   R: sys1 sys2 --       drop loop parms
// U<     u1 u2 -- flag         test u1<n2, unsigned
// VARIABLE   --             define a Forth variable
// XOR    x1 x2 -- x3                    logical XOR
head(XOR,XOR): BINOP(^)

//
//                ANS Forth Extensions
// These are optional words whose definitions are
// specified by the ANS Forth document.
//
// <>     x1 x2 -- flag               test not equal
head(CMP_NE,<>): BINOP(<>)

// BYE    i*x --                      return to CP/M
head(BYE,BYE):
    .word . + 1
    PSP <- [reloc(_PSPinit)]
    RSP <- [reloc(_RSPinit)]
    illegal

// CMOVE  c-addr1 c-addr2 u --      move from bottom
// CMOVE> c-addr1 c-addr2 u --         move from top
// KEY?   -- flag        return true if char waiting
// M+     d1 n -- d2            add single to double
// NIP    x1 x2 -- x2              per stack diagram
head(NIP,NIP):
    .word . + 1
    W   <- [PSP + 1]
    W   -> [PSP + 2]
    PSP <-  PSP + 1
    goto(NEXT)

// TUCK   x1 x2 -- x2 x1 x2        per stack diagram
head(TUCK,TUCK):
    .word . + 1
    T0  <- [PSP + 2]
    T1  <- [PSP + 1]

    PSP <- PSP - 1

    T1  -> [PSP + 3]
    T0  -> [PSP + 2]
    T1  -> [PSP + 1]
    goto(NEXT)

// U>     u1 u2 -- flag         test u1>u2, unsigned

// extensions (possibly borrowed from CamelForth)
// LIT    -- x         fetch inline literal to stack
head(LIT,LIT):
    .word . + 1
    W   <- [IP]
    IP  <- IP + 1
    W   -> [PSP]
    PSP <- PSP - 1
    goto(NEXT)

head(NOOP,NOOP):
    .word @ENTER
    .word @EXIT

.global level0_link
.set level0_link, @link

