// Instruction format
// [PPDDZZZZXXXXYYYYffffIIIIIIIIIIII]
// P = 2-bit immediate position selector
// D = 2-bit dereferencing selector
// Z = 4-bit register selector
// X =   "      "        "
// Y =   "      "        "
// f = 4-bit operation code
// I = 12-bit signed two's-complement immediate
// DD = 00 :     Z  <-  ...
//      01 :     Z  -> [...]
//      10 :    [Z] <-  ...
//      11 :     Z  <- [...]
//
// PP = 00 :    [Z] <- [X f Y + I]
//      01 :    [Z] <- [X f I + Y]
//      10 :    [Z] <- [I f X + Y]
//
// [PPDDZZZZXXXXIIIIIIIIIIIIIIIIIIII]
// I = sign-extended 20-bit immediate
// PP = 11 :    [Z] <- [X     + I]

#ifndef OPS_H_
#define OPS_H_

#include <stdint.h>

#define SMALL_IMMEDIATE_BITWIDTH    12
#define MEDIUM_IMMEDIATE_BITWIDTH   20
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
    int32_t reladdr;    ///< used for CE_ICI resolving
    int32_t size;       ///< used for data (e.g., .zero 5 -> size == 5)
};

struct element_list {
    struct element *elem;
    struct element_list *next, *tail; ///< tail is the last non-null element, possibly self
};

/*
 * Operations are encoded this way to group hardware-similar operations
 * together, differing by the most sigificant bit only.
 *
 * In the table below, the column header is read before the row ; e.g., `&` is
 * 0b0001 and `-` is 0b1100.
 *
 *       0   1
 * 000   |   |~
 * 001   &   &~
 * 010   ^   ^^
 * 011   >>  >>>
 * 100   +   -
 * 101   *   <<
 * 110   ==  @
 * 111   <   >=
 */

enum op {
    OP_BITWISE_OR        = 0x0, OP_BITWISE_ORN       = 0x8,
    OP_BITWISE_AND       = 0x1, OP_BITWISE_ANDN      = 0x9,
    OP_BITWISE_XOR       = 0x2, OP_PACK              = 0xa,
    OP_SHIFT_RIGHT_ARITH = 0x3, OP_SHIFT_RIGHT_LOGIC = 0xb,

    OP_ADD               = 0x4, OP_SUBTRACT          = 0xc,
    OP_MULTIPLY          = 0x5, OP_SHIFT_LEFT        = 0xd,
    OP_COMPARE_EQ        = 0x6, OP_TEST_BIT          = 0xe,
    OP_COMPARE_LT        = 0x7, OP_COMPARE_GE        = 0xf,
};

#endif

/* vi: set ts=4 sw=4 et: */
