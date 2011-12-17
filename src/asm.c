#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ops.h"
#include "asm.h"
#include "common.h"
#include "parser_global.h"

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
    [OP_ADD_NEGATIVE_Y     ] = "-",
    [OP_XOR_INVERT_X       ] = "^~",
    [OP_SHIFT_RIGHT_LOGICAL] = ">>",
    [OP_COMPARE_GT         ] = ">",
    [OP_COMPARE_NE         ] = "<>",
};

int print_disassembly(FILE *out, struct instruction *i)
{
    int type = i->u._xxxx.t;
    switch (type) {
        case 0b1010:
        case 0b1000:
        case 0b1011:
        case 0b1001: {
            struct instruction_load_immediate *g = &i->u._10xx;
            fprintf(out, "%c%c%c <-  0x%08x",
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
            fprintf(out,
                    g->imm ? "%c%c%c %s %c%c %-2s %c + 0x%08x%c"
                           : "%c%c%c %s %c%c %-2s %c%10$c",
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
        default:
            fprintf(out, ".word 0x%08x", i->u.word);
            return 0;
    }

    return -1;
}

int print_registers(FILE *out, int32_t regs[16])
{
    int i = 0;
    for (; i < 6; i++)
        fprintf(out, "%c %08x ", 'A' + i, regs[i]);
    fputs("\n", out);

    for (; i < 12; i++)
        fprintf(out, "%c %08x ", 'A' + i, regs[i]);
    fputs("\n", out);

    for (; i < 16; i++)
        fprintf(out, "%c %08x ", 'A' + i, regs[i]);
    fputs("\n", out);

    return 0;
}

int find_format_by_name(const void *_a, const void *_b)
{
    const struct format *a = _a, *b = _b;
    return strcmp(a->name, b->name);
}

static int binary_in(FILE *in, struct instruction *i)
{
    return fread(&i->u.word, 4, 1, in) == 1;
}

static int text_in(FILE *in, struct instruction *i)
{
    return fscanf(in, "%x", &i->u.word) == 1;
}

static int binary_out(FILE *stream, struct instruction *i)
{
    return fwrite(&i->u.word, sizeof i->u.word, 1, stream) == 1;
}

static int text_out(FILE *stream, struct instruction *i)
{
    return fprintf(stream, "0x%08x\n", i->u.word) > 0;
}

const struct format formats[] = {
    { "binary", binary_in, binary_out },
    { "text"  , text_in,   text_out   },
};

const size_t formats_count = countof(formats);

