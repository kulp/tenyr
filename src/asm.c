#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "obj.h"
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

/*
 * Object format : simple section-based objects
 */
struct obj_fdata {
    int flags;
    struct obj *o;
    long words;
    long insns;
    long syms;
    size_t size;    ///< bytes size of `o'

    struct objsym *last;
    struct objrec *curr;
    uint32_t pos;   ///< position in objrec
};

static int obj_init(FILE *stream, int flags, void **ud)
{
    int rc = 0;

    struct obj_fdata *u = *ud = calloc(1, sizeof *u);
    struct obj_v0 *o = (void*)(u->o = calloc(1, sizeof *o));

    u->flags = flags;

    if (flags & ASM_ASSEMBLE) {
        o->rec_count = 1;
        o->records = calloc(o->rec_count, sizeof *o->records);
        o->records->addr = 0;
        o->records->size = 1024;
        o->records->data = calloc(o->records->size, sizeof *o->records->data);

        o->sym_count = 32;
        o->symbols = calloc(o->sym_count, sizeof *o->records);
    } else if (flags & ASM_DISASSEMBLE) {
        rc = obj_read(u->o, &u->size, stream);
        u->curr = o->records;
    }

    return rc;
}

static int obj_in(FILE *stream, struct instruction *i, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;

    struct objrec *rec = u->curr;
    if (!rec)
        return -1;

    if (u->pos >= rec->size) {
        rec = rec->next;
        u->pos = 0;

        if (!rec) {
            // TODO get symbols
            return -1;
        }
    }

    i->u.word = rec->data[u->pos++];
    // TODO adjust addr where ?
    i->reladdr = rec->addr;
    i->label = NULL;

    return rc;
}

static int obj_out(FILE *stream, struct instruction *i, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;
    struct obj_v0 *o = (void*)u->o;

    {
        if (u->insns >= o->records->size) {
            while (u->insns >= o->records->size)
                o->records->size *= 2;

            o->records->data = realloc(o->records->data,
                    o->records->size * sizeof *o->records->data);
        }

        o->records->data[u->insns] = i->u.word;

        u->words++;
        u->insns++;
    }

    {
        list_foreach(label, Node, i->label) {
            if (Node->global) {
                if (u->syms >= o->sym_count) {
                    while (u->syms >= o->sym_count)
                        o->sym_count *= 2;

                    o->symbols = realloc(o->symbols,
                                            o->sym_count * sizeof *o->symbols);
                }

                struct objsym *sym = &o->symbols[u->syms++];
                strncpy(sym->name, Node->name, sizeof sym->name);
                assert(("Symbol address resolved", Node->resolved != 0));
                sym->value = Node->reladdr;
                if (u->last) u->last->next = sym;
                sym->prev = u->last;
                u->last = sym;

                u->words++;
            }
        }
    }

    return rc;
}

static int obj_fini(FILE *stream, void **ud)
{
    int rc = 0;

    struct obj_fdata *u = *ud;
    struct obj_v0 *o = (void*)u->o;

    if (u->flags & ASM_ASSEMBLE) {
        o->records->size = u->insns;
        o->sym_count = u->syms;
        o->length = 5 + u->words; // XXX explain

        obj_write(u->o, stream);
    }

    obj_free(u->o);

    free(*ud);
    *ud = NULL;

    return rc;
}

/*
 * Raw format : raw binary data (host endian)
 */
static int raw_in(FILE *stream, struct instruction *i, void *ud)
{
    return fread(&i->u.word, 4, 1, stream) == 1;
}

static int raw_out(FILE *stream, struct instruction *i, void *ud)
{
    return fwrite(&i->u.word, sizeof i->u.word, 1, stream) == 1;
}

/*
 * Text format : hexadecimal numbers
 */
static int text_in(FILE *stream, struct instruction *i, void *ud)
{
    return fscanf(stream, "%x", &i->u.word) == 1;
}

static int text_out(FILE *stream, struct instruction *i, void *ud)
{
    return fprintf(stream, "0x%08x\n", i->u.word) > 0;
}

const struct format formats[] = {
    // first format is default
    { "obj"   , obj_init, obj_in , obj_out , obj_fini },
    { "raw"   , NULL    , raw_in , raw_out , NULL     },
    { "text"  , NULL    , text_in, text_out, NULL     },
};

const size_t formats_count = countof(formats);

