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

    [OP_RESERVED           ] = "XX",
};

int print_disassembly(FILE *out, struct instruction *i)
{
    int type = i->u._xxxx.t;
    switch (type) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: {
            struct instruction_general *g = &i->u._0xxx;
            int ld = g->dd & 2;
            int rd = g->dd & 1;
            int noop = g->y == 0 && g->op == OP_BITWISE_OR;
            int imm = g->imm;

            // LHS
                  char  f0 = ld ? '[' : ' ';        // left side dereferenced ?
                  char  f1 = 'A' + g->z;            // register name for Z
                  char  f2 = ld ? ']' : ' ';        // left side dereferenced ?

            // arrow
            const char *f3 = g->r ? "->" : "<-";    // arrow direction

            // RHS
                  char  f4 = rd ? '[' : ' ';        // right side dereferenced ?
                  char  f5 = 'A' + g->x;            // register name for X
            const char *f6 = op_names[g->op];       // operator name
                  char  f7 = 'A' + g->y;            // register name for Y
                  int   f8 = g->imm;                // immediate value
                  char  f9 = rd ? ']' : ' ';        // right side dereferenced ?

            // argument placement :         f0f1f2 f3 f4f5   f6 f7       f8f9
            static const char  imm_nop[] = "%c%c%c %s %c%c "      "+ 0x%08x%c";
            static const char  imm_op [] = "%c%c%c %s %c%c %-2s %c + 0x%08x%c";
            static const char nimm_nop[] = "%c%c%c %s %c%c"               "%c";
            static const char nimm_op [] = "%c%c%c %s %c%c %-2s %c"       "%c";

            imm ? noop ? fprintf(out,  imm_nop, f0,f1,f2,f3,f4,f5,      f8,f9)
                       : fprintf(out,  imm_op , f0,f1,f2,f3,f4,f5,f6,f7,f8,f9)
                : noop ? fprintf(out, nimm_nop, f0,f1,f2,f3,f4,f5,         f9)
                       : fprintf(out, nimm_op , f0,f1,f2,f3,f4,f5,f6,f7,   f9);

            return 0;
        }
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb: {
            struct instruction_load_immediate *g = &i->u._10xx;
            int ld = g->dd & 2;
            int rd = g->dd & 1;
            fprintf(out, "%c%c%c <- %c0x%08x%c",
                    ld ? '[' : ' ', // left side dereferenced ?
                    'A' + g->z,     // register name for Z
                    ld ? ']' : ' ', // left side dereferenced ?
                    rd ? '[' : ' ', // right side dereferenced ?
                    g->imm,         // immediate value
                    rd ? ']' : ' '  // right side dereferenced ?
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

