// Instructions
// [TT..............................] TT = instruction type/class
// all-zero instruction = no-op
//  a <- a | a + 0 (and A has sticky value zero and takes no writes)
//
// algebraic assembler
//
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
// [10DDZZZZXXXXYYYYffffIIIIIIIIIIII]
// I = sign-extended 12-bit immediate
// performs
// DD = 00 :  Z  <-  I f X + Y
//      10 : [Z] <-  I f X + Y
//      01 :  Z  <- [I f X + Y]
//      11 :  Z  -> [I f X + Y]
//
// [11DDZZZZIIIIIIIIIIIIIIIIIIIIIIII]
// I = sign-extended 24-bit immediate
// performs
// DD = 00 :  Z  <-  I
//      10 : [Z] <-  I
//      01 :  Z  <- [I]
//      11 :  Z  -> [I]
//
//  a <- [b * c + 4]
//  a <- [p + 3]
//  p <- [p + c] (jump from table)
//  c <- c == a (is zero ?)
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
//  0110 = X compare < Y
//  0111 = X compare == Y
//  1000 = X compare >= Y
//  1001 = X bitwise and complement Y
//  1010 = X bitwise xor Y
//  1011 = X subtract Y
//  1100 = X xor ones' complement Y
//  1101 = X shift right logical Y
//  1110 = X compare != Y
//  1111 = X shift right arithmetic Y

#ifndef OPS_H_
#define OPS_H_

#include "common.h"

#include <stdint.h>

#define SMALL_IMMEDIATE_BITWIDTH    12
#define MEDIUM_IMMEDIATE_BITWIDTH   24
#define WORD_BITWIDTH               32

// TODO assumes bits are filled in rightmost-first
struct insn_or_data {
    union insn {
        uint32_t word;
        struct instruction_typeany {
            unsigned     : 20;   ///< differs by type
            unsigned x   :  4;   ///< operand x
            unsigned z   :  4;   ///< operand z
            unsigned dd  :  2;   ///< dereference
            unsigned p   :  2;   ///< type code
        } typeany;
        struct instruction_type012 {
            unsigned imm : SMALL_IMMEDIATE_BITWIDTH; ///< immediate
            unsigned op  :  4;  ///< operation
            unsigned y   :  4;  ///< operand y
            unsigned x   :  4;  ///< operand x
            unsigned z   :  4;  ///< operand z
            unsigned dd  :  2;  ///< dereference
            unsigned p   :  2;  ///< type code
        } type012;
        struct instruction_type3 {
            unsigned imm : MEDIUM_IMMEDIATE_BITWIDTH; ///< immediate
            unsigned x   :  4;  ///< operand x
            unsigned z   :  4;  ///< operand z
            unsigned dd  :  2;  ///< dereference
            unsigned p   :  2;  ///< expr type0 or type1
        } type3;
    } u;
    uint32_t reladdr;   ///< used for CE_ICI resolving
    uint32_t size;      ///< unused for instruction, used for data
};

struct element {
    struct insn_or_data insn;

    struct symbol {
        char name[SYMBOL_LEN];
        int column;
        int lineno;
        int32_t reladdr;
        uint32_t size;

        unsigned resolved:1;
        unsigned global:1;
        unsigned unique:1;  ///< if this symbol comes from a label

        struct const_expr *ce;

        struct symbol *next;
    } *symbol;
    struct reloc_node *reloc;
};

struct element_list {
    struct element *elem;
    struct element_list *prev, *next;
};

enum op {
    OP_BITWISE_OR        = 0x0,
    OP_BITWISE_AND       = 0x1,
    OP_ADD               = 0x2,
    OP_MULTIPLY          = 0x3,
    OP_PACK              = 0x4,
    OP_SHIFT_LEFT        = 0x5,
    OP_COMPARE_LT        = 0x6,
    OP_COMPARE_EQ        = 0x7,
    OP_COMPARE_GE        = 0x8,
    OP_BITWISE_ANDN      = 0x9,
    OP_BITWISE_XOR       = 0xa,
    OP_SUBTRACT          = 0xb,
    OP_BITWISE_XORN      = 0xc,
    OP_SHIFT_RIGHT_LOGIC = 0xd,
    OP_COMPARE_NE        = 0xe,
    OP_SHIFT_RIGHT_ARITH = 0xf,
};

#endif

/* vi: set ts=4 sw=4 et: */
