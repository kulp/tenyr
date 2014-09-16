# tenyr 
[![Build Status](https://travis-ci.org/kulp/tenyr.svg?branch=develop)](https://travis-ci.org/kulp/tenyr)

## Overview

**tenyr** is a 32-bit computer architecture and computing environment that
focuses on simplicity of design and implementation. **tenyr** comprises :

* an [instruction set architecture (ISA)](https://github.com/kulp/tenyr/wiki/Assembly-language)
* an [implementation in FPGA hardware](https://github.com/kulp/tenyr/tree/develop/hw/verilog) with device support
* tools for building software
  * [assembler (tas)](https://github.com/kulp/tenyr/wiki/Assembler)
  * linker (tld)
  * [simulator (tsim)](https://github.com/kulp/tenyr/wiki/Simulator)
* a [standard library](https://github.com/kulp/tenyr/tree/develop/lib) of tenyr code
* some [example software](https://github.com/kulp/tenyr/tree/develop/ex), including :
  * a [timer](https://github.com/kulp/tenyr/blob/develop/ex/clock.tas.cpp)
  * [random snakes](https://github.com/kulp/tenyr/blob/develop/ex/bm_snake.tas.cpp)
  * a [recursive Fibonacci number generator](https://github.com/kulp/tenyr/blob/develop/ex/bm_fib.tas.cpp)
  
Someday it will also include :

* a C compiler based on LCC
* [a Forth environment](https://github.com/kulp/tenyr/tree/develop/forth)
* a novel operating system

Explore a [simple online demo of the **tenyr** toolset](http://demo.tenyr.info/).

This page serves as a general overview of the design of **tenyr**. It
is not an exhaustive reference. An older version of some of this
documentation, perhaps in some ways more complete, can be found [on the
**tenyr** wiki](https://github.com/kulp/tenyr/wiki).

---

## Description

### Assembler Syntax and Machine Model

#### Registers

**tenyr** has sixteen registers, named `A` through `P`. Refer to them
using their uppercase or lowercase names ; **tenyr** tools are not
case-sensitive. Two of these registers, `A` and `P`, are special, while
the others are completely general purpose. All registers are 32 bits
wide, and are treated as two's-complement signed integers everywhere
that makes sense (there are no unsigned 32-bit integers in **tenyr**,
just signed integers and bitstrings).

#### Instruction Format

Every **tenyr** instruction can be expressed in a single, regular,
algebraic instruction format :

```
Z <- _ op _ + _
```

where `Z` is a register, `op` is one of the accepted arithmetic
operations, `+` is addition, two of the blanks are registers, and
one of the blanks is an immediate value (a integer) between -2048 and
2047. Any one of the blanks and the operation that goes before it can
be left out. Examples :

```
a <- a + a + 0
b <- c * d + 3
c <- d - e + -2
d <- e ^ f
e <- f - 2
f <- 2 | g
g <- -h
h <- i <> 0
i <- j < k
j <- k
```

#### Operations

Here are the operations that **tenyr** supports :

 Syntax         | Description
:--------------:|:-------------------
 <code>B <- C  &#124;  D</code> | C bitwise or D
 `B <- C  &  D` | C bitwise and D
 `B <- C  +  D` | C add D
 `B <- C  *  D` | C multiply D
 `B <- C <<  D` | C shift left D
 `B <- C  <  D` | C test less than D
 `B <- C ==  D` | C test equal to D
 `B <- C >=  D` | C test greater than or equal to D
 `B <- C  &~ D` | C bitwise and complement D
 `B <- C  ^  D` | C bitwise xor D
 `B <- C  -  D` | C subtract D
 `B <- C  ^~ D` | C xor ones' complement D
 `B <- C >>  D` | C shift right logical D
 `B <- C <>  D` | C test not equal to D
 `B <- C >>> D` | C shift right arithmetic D

Some of the operations merit explanation. The comparison operations
(`<`, `>=`, `==`, and `<>`) produce a result that is either `0` (false)
or `-1` (true). The canonical truth value in **tenyr** is `-1`, not
`1`. This allows us to do clever things with masks, and also explains
the existence of the special `&~` and `^~` operations -- when the second
operand is a truth value, the bitwise complement works as a Boolean
NOT. The operations also underlie some syntactical sugar ; for example,
`B <- ~C` is accepted by the assembler and transformed into `B <- C ^~ A`.

#### Memory Operations

A memory operation looks just like a register-register operation, but
with one side of the instruction dereferenced, using brackets :

```
 d  <- [e * 4 + f]    // a load into D
 e  -> [f << 2]       // a store from E
[f] <- 2              // another kind of store, with an immediate value
```

One instruction can't have brackets on both sides of an arrow, and an
immediate value cannot appear on the left side of an arrow. Otherwise,
most anything that makes sense using the operations that are valid in
**tenyr** should be possible.

#### Instruction Shorthand

Although pieces of the right-hand-side of an instruction can be left
out during assembly, under the covers all the pieces are still there ;
the missing parts are filled in with zeros or with references to the
special `A` register, which always contains `0`, even if it is written
to. Therefore, each instruction in the following pairs is identical to
the other one in the pair :

```
b <- 3      ; b <- a +  a + 3
c <- d *  e ; c <- d *  e + 0
e <- 1 << b ; e <- 1 << b + a
```

To see the expanded form, invoke the disassembler (`tas -d`) with the
`-v` option.

#### Control Flow

**tenyr** has no dedicated control-flow instructions ; flow is
controlled by updating the `P` register, which is the program counter /
instruction pointer. Reading from `P` will produce the address of the
currently executing instruction, plus one. Writing to it will cause the
next instruction executed to be fetched from the address written into
`P`. For example, if this program starts at address 0 :

```
B <- P        // after this instruction, B contains 1
D <- 3        // after this instruction, D contains 3
P <- P - 3    // this is a loop back to the first instruction above
```

Notice that in the third instruction it was necessary to subtract 3
instead of 2, because the value in `P` was effectively the location of
the *next* instruction that would have been executed in the absence of
a control flow change.

Under normal circumstances, the programmer is not expected
to update the `P` register in such a direct fashion,
but rather to use a macro like `jnzrel(reg,target)` from
[common.th](https://github.com/kulp/tenyr/blob/develop/lib/common.th) :

```
    D <- 5
    C <- 10
loop_top:
    C < C - 1
    N <- C > D
    jnzrel(N,loop_top)
```

where `loop_top` is a label to jump to, and `jnzrel` means "jump if not
zero to relative" (admittedly, this is not a very good name, because
`N` needs to be `-1` not merely nonzero).

Notice that we used `>` even though this is not one of the supported
operations. The assembler accepts `>` and rewrites it into a valid
**tenyr** instruction by swapping the order of the operands and using
`<` instead. An analogous transformation occurs for `<=`.

#### Special instructions

There is currently one special instruction, written as `illegal`
; it stops program execution in the simulator. It is currently not
implemented in hardware (use an infinite loop instead, such as `P <- P -
1`). Eventually it will be a specific case of a more generic exception
mechanism, but for now it is the only "trap" instruction **tenyr** has.

#### Labels

Labels can be used to identify segments of code and data. A label
is defined by a sequence of alphanumeric characters and underscores,
where that sequence cannot look like a register name (this restriction
may be relaxed in the future). A label is referred to by prefixing `@`
to its name :

```
data:
    .word 0xdeadbeef
top:
    B <- C
    D <- @data
    E <- @top
```

Getting the value of `@label` isn't very useful, though, because
that value is relative to where the code was loaded in memory. So if
code was loaded at the default address of `0x1000`, one would need to
add `0x1000` to `@data` to get the absolute value in memory. This is
easier when using the special label `.` ; then the expression `P - .`
will be the loading offset. This is handled by the `rel()` macro from
[common.th](https://github.com/kulp/tenyr/blob/develop/lib/common.th).
Think of `rel()` as producing a "relocated" address from a relative one.

#### Immediates

Immediate values are 12 bits wide, thus ranging from -2048 to 2047. ASCII
(the ASCII subset of UTF-32, really) character constants can appear in
immediate expressions :

```
B <- '$'
C <- 4
```

An immediate value can also be an expression with multiple terms, as
long as :

1. all of the terms are constants
2. the entire expression is enclosed in parentheses
3. a `@label` reference occurs at most once

The result of an immediate expression is computed in the assembler,
and only the resulting immediate value is written out. Many of the
**tenyr** operations can be used in immediate expressions, too, as well
as one that isn't available : integer division, with `/`. Be aware that
currently there is no operator precedence within an immediate expression ;
expressions are evaluated left-to-right.

```
B <- B ^ ('A' ^ 'a')  // flip capitalisation of the character in B
C <- ((124 - 1) | 1)  // after this instruction, C will contain the value 123
D <- (8 / 4)          // D will contain 2
E <- (16 - 8 / 4)     // E will contain 2, not 14
```

#### Directives

There are a few assembly directives to make assembly easier :

```
.word 0, 1, 2, 0x1234, 'A', 'B'    // each value is expanded to a 32-bit word
.utf32 "Hello, world"              // each character is saved in a 32-bit word
.zero 0x14                         // this creates 0x14 = 20 zeros in a row
```

### Caveats

It is intended that disassembling a program and reassembling it will
produce an identical binary, but this is not yet guaranteed. Because
most word-sized bit-patterns are valid **tenyr** instructions, and
because the default disassembly of a constant value might be reassembled
as a different value, it is recommended to use `-v` to produce verbose
disassembly if reassembly is intended. File a bug on any situation where
an assembler-disassembler-assembler round-trip does not produce identical
output on each round.
