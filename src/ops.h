// Instructions
// [TTTT............................] TTTT = instruction type/class
// all-zero instruction = no-op
//  a <- a | a + 0 (and A has sticky value zero and takes no writes)
// all-one instruction = illegal instruction
// TTTT == 1111 : reserved
//
// algebraic assembler
//
// load/store/arith/control (TTTT = 0xxx)
// [00DDZZZZXXXXYYYYffffIIIIIIIIIIII]
// I = sign-extended 12-bit immediate
// performs
// DD = 00 :  Z  <-  X f Y + I
//      10 : [Z] <-  X f Y + I
//      01 :  Z  <- [X f Y + I]
//      11 :  Z  -> [X f Y + I]
//
// [01DDZZZZXXXXYYYYffffIIIIIIIIIIII]
// I = sign-extended 12-bit immediate
// performs
// DD = 00 :  Z  <-  X f I + Y
//      10 : [Z] <-  X f I + Y
//      01 :  Z  <- [X f I + Y]
//      11 :  Z  -> [X f I + Y]
//
//  a <- [b * c + 4]
//  a <- [p + 3]
//  p <- [p + c] (jump from table)
//  c <- c <> a (nonzero)
//  d <- c > d (no status flags, just bool result)
//  e <- d >> e
//  p <- p + -4 (jump backward)
// ops
//  0000 = X bitwise or Y
//  0001 = X bitwise and Y
//  0010 = X add Y
//  0011 = X multiply Y
//  0100 = reserved
//  0101 = X shift left Y
//  0110 = X compare <= Y
//  0111 = X compare == Y
//  1000 = X bitwise nor Y
//  1001 = X bitwise nand Y
//  1010 = X bitwise xor Y
//  1011 = X add two's complement Y
//  1100 = X xor ones' complement Y
//  1101 = X shift right logical Y
//  1110 = X compare > Y
//  1111 = X compare <> Y

#ifndef OPS_H_
#define OPS_H_

#include "common.h"

#include <stdint.h>

// TODO assumes bits are filled in rightmost-first
struct instruction {
    union {
        uint32_t word;
        struct instruction_any {
            unsigned   : 28;    ///< unused
            unsigned t :  4;    ///< type code
        } _xxxx;
        struct instruction_general {
            unsigned imm : 12;  ///< immediate
            unsigned op  :  4;  ///< operation
            unsigned y   :  4;  ///< operand y
            unsigned x   :  4;  ///< operand x
            unsigned z   :  4;  ///< operand z
            unsigned dd  :  2;  ///< dereference
            unsigned p   :  1;  ///< expr type0 or type1
            unsigned t   :  1;  ///< type bit
        } _0xxx;
    } u;
    uint32_t reladdr;    // used for CE_ICI resolving
    struct symbol {
        char name[SYMBOL_LEN];
        int column;
        int lineno;
        uint32_t reladdr;

        unsigned resolved:1;
        unsigned global:1;
        unsigned unique:1;  ///< if this symbol comes from a label

        struct const_expr *ce;

        struct symbol *next;
    } *symbol;
    struct reloc_node *reloc;
};

struct instruction_list {
    struct instruction *insn;
    struct instruction_list *prev, *next;
};

enum op {
    OP_BITWISE_OR          = 0x0,
    OP_BITWISE_AND         = 0x1,
    OP_ADD                 = 0x2,
    OP_MULTIPLY            = 0x3,
    OP_RESERVED            = 0x4,
    OP_SHIFT_LEFT          = 0x5,
    OP_COMPARE_LTE         = 0x6,
    OP_COMPARE_EQ          = 0x7,
    OP_BITWISE_NOR         = 0x8 | OP_BITWISE_OR,
    OP_BITWISE_NAND        = 0x8 | OP_BITWISE_AND,
    OP_BITWISE_XOR         = 0xa,
    OP_ADD_NEGATIVE_Y      = 0xb,
    OP_XOR_INVERT_X        = 0xc,
    OP_SHIFT_RIGHT_LOGICAL = 0xd,
    OP_COMPARE_GT          = 0x8 | OP_COMPARE_LTE,
    OP_COMPARE_NE          = 0x8 | OP_COMPARE_EQ,
};

#endif

