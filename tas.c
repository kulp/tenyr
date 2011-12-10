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
    [OP_COMPARE_NE         ] = "<>",
};

int print_disassembly(struct instruction *i)
{
    switch (i->u._xxxx.t) {
        case 0b1010:
        case 0b1000: {
            struct instruction_load_immediate_unsigned *g = &i->u._10x0;
            printf("%c%c%c <-  0x%08x\n",
                    g->d ? '[' : ' ',   // left side dereferenced ?
                    'A' + g->z,         // register name for Z
                    g->d ? ']' : ' ',   // left side dereferenced ?
                    g->imm              // immediate value
                );
            return 0;
        }
        case 0b1011:
        case 0b1001: {
            struct instruction_load_immediate_signed *g = &i->u._10x1;
            printf("%c%c%c <-  0x%08x\n",
                    g->d ? '[' : ' ',   // left side dereferenced ?
                    'A' + g->z,         // register name for Z
                    g->d ? ']' : ' ',   // left side dereferenced ?
                    g->imm              // immediate value
                );
            return 0;
        }
        case 0b0000 ... 0b0111: {
            struct instruction_general *g = &i->u._0xxx;
            int ld = g->dd & 2;
            int rd = g->dd & 1;
            printf("%c%c%c %s %c%c %-3s %c + 0x%08x%c\n",
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

            return 0;
        }
    }

    return -1;
}

int main(int argc, char *argv[])
{
    print_disassembly(&(struct instruction){ 0x12345678 });
    print_disassembly(&(struct instruction){ 0x80f23456 });
    print_disassembly(&(struct instruction){ 0x90f23456 });
    print_disassembly(&(struct instruction){ 0xa0f23456 });
    print_disassembly(&(struct instruction){ 0xb0f23456 });
    print_disassembly(&(struct instruction){ 0x7fedc000 });

    int result = yyparse();

    return 0;
}

