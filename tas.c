#include <stdlib.h>
#include <stdio.h>

#include "ops.h"

static const char *op_names[] = {
    [OP_BITWISE_OR         ] = "|",
    [OP_BITWISE_AND        ] = "&",
    [OP_ADD                ] = "+",
    [OP_MULTIPLY           ] = "*",
    [OP_MODULUS            ] = "%",
    [OP_SHIFT_LEFT         ] = "<<",
    [OP_COMPARE_LTE        ] = "<=",
    [OP_COMPARE_EQ         ] = "==",
    [OP_BITWISE_NOR        ] = "~|",
    [OP_BITWISE_NAND       ] = "~&",
    [OP_BITWISE_XOR        ] = "^",
    [OP_ADD_NEGATIVE_Y     ] = "+ -",
    [OP_XOR_INVERT_X       ] = "^ ~",
    [OP_SHIFT_RIGHT_LOGICAL] = ">>",
    [OP_COMPARE_GT         ] = ">",
    [OP_COMPARE_NE         ] = "!=",
};

int print_disassembly(struct instruction *i)
{
    switch (i->u._xxxx.t) {
        case 0x8:
        case 0x9:
            // TODO handle immediate load
            return -1;
        case 0xa:
            // TODO handle immediate load high
            return -1;
        case 0b0000 ... 0b0111: {
            struct instruction_general *g = &i->u._0xxx;
            int ld = g->dd & 2;
            int rd = g->dd & 1;
            printf("%c%c%c %s %c%c %s %c + 0x%x%c\n",
                    ld ? '[' : ' ',     // left side dereferenced ?
                    'A' + g->z,         // register name for Z
                    ld ? ']' : ' ',     // left side dereferenced ?
                    g->r ? "->" : "<-", // arrow direction
                    rd ? '[' : ' ',     // right side dereferenced ?
                    'A' + g->x,         // register name for X
                    op_names[g->op],    // operator name
                    'A' + g->y,         // register name for Y
                    g->imm,             // immediate value
                    rd ? ']' : ' '      // right side dereferenced ?
                );

            return -1;
        }
    }

    return -1;
}

int main(int argc, char *argv[])
{
    struct instruction insn = { .u.word = 0x12345678 };

    print_disassembly(&insn);

    return 0;
}

