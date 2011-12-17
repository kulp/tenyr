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
// [0rDDZZZZXXXXYYYYffffIIIIIIIIIIII]
// I = sign-extended 12-bit immediate
// r =   0 : <-
//       1 : ->
// performs
// DD = 00 :  Z  r  X f Y + I
//      01 :  Z  r [X f Y + I]
//      10 : [Z] r  X f Y + I
//      11 : [Z] r [X f Y + I]
// rDD = 1x0 : invalid but legal (reads, discards)
//
// immediate load (TTTT = 100x)
// [10DpZZZZJJJJJJJJJJJJJJJJJJJJJJJJ]
// J = 24-bit immediate
// performs
// Dp = 00 :  Z  <- J (zero-extended)
//      01 :  Z  <- J (sign-extended)
//      10 : [Z] <- J (zero-extended)
//      11 : [Z] <- J (sign-extended)
//
//  a <- [b * c + 4]
//  [p + 3] -> a
//  [p + c] -> p (jump from table)
//  c <- c <> a (nonzero)
//  d <- c > d (no status flags, just bool result)
//  ri <- ri + c
//  rv <- ri % 2
//  d >> e -> e
//  e <- d >> e
//  p <- p + -4 (jump backward)
// ops
//  0000 = X bitwise or Y
//  0001 = X bitwise and Y
//  0010 = X add Y
//  0011 = X multiply Y
//  0100 = X modulus Y
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

#include <stdint.h>

// TODO assumes bits are filled in rightmost-first
struct instruction {
    uint32_t reladdr;    // used for ICI resolving
    struct label {
        char name[32]; // TODO document restriction
        int column;
        int lineno;
        uint32_t reladdr;
        int resolved;

        struct label *next;
    } *label;
    union {
        uint32_t word;
        struct instruction_any {
            unsigned   : 28;    ///< unused
            unsigned t :  4;    ///< type code
        } _xxxx;
        struct instruction_load_immediate_unsigned {
            unsigned imm : 24;  ///< immediate
            unsigned z   :  4;  ///< destination
            unsigned p   :  1;  ///< extension mode
            unsigned d   :  1;  ///< dereference
            unsigned t   :  2;  ///< type bits
        } _10x0;
        struct instruction_load_immediate_signed {
            signed   imm : 24;  ///< immediate
            unsigned z   :  4;  ///< destination
            unsigned p   :  1;  ///< extension mode
            unsigned d   :  1;  ///< dereference
            unsigned t   :  2;  ///< type bits
        } _10x1;
        struct instruction_general {
            signed   imm : 12;  ///< immediate
            unsigned op  :  4;  ///< operation
            unsigned y   :  4;  ///< operand y
            unsigned x   :  4;  ///< operand x
            unsigned z   :  4;  ///< operand z
            unsigned dd  :  2;  ///< dereference
            unsigned r   :  1;  ///< reverse
            unsigned t   :  1;  ///< type bit
        } _0xxx;
    } u;
};

struct instruction_list {
    struct instruction *insn;
    struct instruction_list *next;
};

enum op {
    OP_BITWISE_OR          = 0x0,
    OP_BITWISE_AND         = 0x1,
    OP_ADD                 = 0x2,
    OP_MULTIPLY            = 0x3,
    OP_MODULUS             = 0x4, // XXX modulus is expensive ; might as well provide division then
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

