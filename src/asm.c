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

int print_disassembly(FILE *out, struct instruction *i, int flags)
{
    if (flags & ASM_AS_DATA)
        return fprintf(out, ".word 0x%08x", i->u.word);

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
            int rd = g->dd &  1;
            int ld = g->dd == 2;

            // LHS
                  char  f0 = ld ? '[' : ' ';        // left side dereferenced ?
                  char  f1 = 'A' + g->z;            // register name for Z
                  char  f2 = ld ? ']' : ' ';        // left side dereferenced ?

            // arrow
            const char *f3 = (g->dd == 3) ? "->" : "<-";    // arrow direction

            // RHS
                  char  f4 = rd ? '[' : ' ';        // right side dereferenced ?
                  char  f5 = 'A' + g->x;            // register name for X
            const char *f6 = op_names[g->op];       // operator name
                  char  f7 = 'A' + g->y;            // register name for Y
                  int   f8 = g->imm;                // immediate value
                  char  f9 = rd ? ']' : ' ';        // right side dereferenced ?

            // indices : [g->p][op1][op2][op3]
            static const char fmts[2][2][2][2][34] = {
                // args :       f0f1f2 f3 f4f5   f6 f7       f8f9
              //[0][0][0][0] = "%c%c%c %s %c"                 "%c", // [Z] <- [           ]
                [0][0][0][1] = "%c%c%c %s %c"           "0x%08x%c", // [Z] <- [        0x0]
                [0][0][1][0] = "%c%c%c %s %c"      "%c"       "%c", // [Z] <- [    Y      ]
                [0][0][1][1] = "%c%c%c %s %c"      "%c + 0x%08x%c", // [Z] <- [    Y + 0x0]
                [0][1][0][0] = "%c%c%c %s %c%c"               "%c", // [Z] <- [X          ]
                [0][1][0][1] = "%c%c%c %s %c%c"      " + 0x%08x%c", // [Z] <- [X     + 0x0]
                [0][1][1][0] = "%c%c%c %s %c%c %-2s %c"       "%c", // [Z] <- [X - Y      ]
                [0][1][1][1] = "%c%c%c %s %c%c %-2s %c + 0x%08x%c", // [Z] <- [X - Y + 0x0]
                // args :       f0f1f2 f3 f4f5   f6 f8       f7f9
              //[1][0][0][0] = "%c%c%c %s %c"                 "%c", // [Z] <- [           ]
                [1][0][0][1] = "%c%c%c %s %c"               "%c%c", // [Z] <- [          Y]
                [1][0][1][0] = "%c%c%c %s %c"      "0x%08x"   "%c", // [Z] <- [    0x0    ]
                [1][0][1][1] = "%c%c%c %s %c"      "0x%08x + %c%c", // [Z] <- [    0x0 + Y]
                [1][1][0][0] = "%c%c%c %s %c%c"               "%c", // [Z] <- [X          ]
                [1][1][0][1] = "%c%c%c %s %c%c"          " + %c%c", // [Z] <- [X       + Y]
                [1][1][1][0] = "%c%c%c %s %c%c %-2s 0x%08x"   "%c", // [Z] <- [X - 0x0    ]
                [1][1][1][1] = "%c%c%c %s %c%c %-2s 0x%08x + %c%c", // [Z] <- [X - 0x0 + Y]
            };

            int op3 = g->p ? !(g->y == 0) : (!!g->imm);
            int op2 = g->op != OP_BITWISE_OR || (g->p ? g->imm : g->y != 0);
            int op1 = !(g->x == 0 && g->op == OP_BITWISE_OR) || (!op2 && !op3);

            #define C_(A,B,C,D) (((A) << 12) | ((B) << 8) | ((C) << 4) | ((D) << 0))
            #define PUT(...) return fprintf(out, fmts[g->p][op1][op2][op3], __VA_ARGS__)

            switch (C_(g->p,op1,op2,op3)) {
              //case C_(0,0,0,0): PUT(f0,f1,f2,f3,f4,            f9); break;
                case C_(0,0,0,1): PUT(f0,f1,f2,f3,f4,         f8,f9); break;
                case C_(0,0,1,0): PUT(f0,f1,f2,f3,f4,      f7,   f9); break;
                case C_(0,0,1,1): PUT(f0,f1,f2,f3,f4,      f7,f8,f9); break;
                case C_(0,1,0,0): PUT(f0,f1,f2,f3,f4,f5,         f9); break;
                case C_(0,1,0,1): PUT(f0,f1,f2,f3,f4,f5,      f8,f9); break;
                case C_(0,1,1,0): PUT(f0,f1,f2,f3,f4,f5,f6,f7,   f9); break;
                case C_(0,1,1,1): PUT(f0,f1,f2,f3,f4,f5,f6,f7,f8,f9); break;

              //case C_(1,0,0,0): PUT(f0,f1,f2,f3,f4,            f9); break;
                case C_(1,0,0,1): PUT(f0,f1,f2,f3,f4,         f7,f9); break;
                case C_(1,0,1,0): PUT(f0,f1,f2,f3,f4,      f8,   f9); break;
                case C_(1,0,1,1): PUT(f0,f1,f2,f3,f4,      f8,f7,f9); break;
                case C_(1,1,0,0): PUT(f0,f1,f2,f3,f4,f5,         f9); break;
                case C_(1,1,0,1): PUT(f0,f1,f2,f3,f4,f5,      f7,f9); break;
                case C_(1,1,1,0): PUT(f0,f1,f2,f3,f4,f5,f6,f8,   f9); break;
                case C_(1,1,1,1): PUT(f0,f1,f2,f3,f4,f5,f6,f8,f7,f9); break;

                default:
                    fatal(0, "Unsupported type/op1/op2/op3 %04x",
                            C_(g->p,op1,op2,op3));
            }

            return 0;
        }
        default:
            if (i->u.word == 0xffffffff)
                return fprintf(out, "illegal");
            else
                return fprintf(out, ".word 0x%08x", i->u.word);
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

/*******************************************************************************
 * Object format : simple section-based objects
 */
struct obj_fdata {
    int flags;
    struct obj *o;
    long words;
    long insns;
    long syms;
    long rlcs;

    struct objrec *curr_rec;

    struct objsym **next_sym;
    struct objrlc **next_rlc;
    uint32_t pos;   ///< position in objrec
};

static int obj_init(FILE *stream, int flags, void **ud)
{
    int rc = 0;

    struct obj_fdata *u = *ud = calloc(1, sizeof *u);
    struct obj *o = u->o = calloc(1, sizeof *o);

    u->flags = flags;

    if (flags & ASM_ASSEMBLE) {
        // TODO proper multiple-records support
        o->rec_count = 1;
        o->records = calloc(o->rec_count, sizeof *o->records);
        o->records->addr = 0;
        o->records->size = 1024;
        o->records->data = calloc(o->records->size, sizeof *o->records->data);
        u->curr_rec = o->records;

        u->next_sym = &o->symbols;
        u->next_rlc = &o->relocs;
    } else if (flags & ASM_DISASSEMBLE) {
        rc = obj_read(u->o, stream);
        u->curr_rec = o->records;
    }

    return rc;
}

static int obj_in(FILE *stream, struct instruction *i, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;

    struct objrec *rec = u->curr_rec;
    if (!rec)
        return -1;

    if (u->pos >= rec->size) {
        u->curr_rec = rec = rec->next;
        u->pos = 0;

        if (!rec) {
            // TODO get symbols
            return -1;
        }
    }

    i->u.word = rec->data[u->pos++];
    // TODO adjust addr where ?
    i->reladdr = rec->addr;
    i->symbol = NULL;

    return rc;
}

static void obj_out_symbols(struct symbol *symbol, struct obj_fdata *u, struct obj *o)
{
    list_foreach(symbol, Node, symbol) {
        if (Node->global) {
            struct objsym *sym = *u->next_sym = calloc(1, sizeof *sym);

            strncpy(sym->name, Node->name, sizeof sym->name);
            assert(("Symbol address resolved", Node->resolved != 0));
            sym->value = Node->reladdr;

            u->next_sym = &sym->next;
            u->syms++;
        }
    }
}

static void obj_out_reloc(struct reloc_node *reloc, struct obj_fdata *u, struct obj *o)
{
    if (!reloc) return;

    struct objrlc *rlc = *u->next_rlc = calloc(1, sizeof *rlc);

    rlc->flags = 0; // TODO
    strncpy(rlc->name, reloc->name, sizeof rlc->name);
    rlc->name[sizeof rlc->name - 1] = 0;
    rlc->addr = reloc->insn->reladdr;
    rlc->width = reloc->width;

    u->next_rlc = &rlc->next;

    u->rlcs++;
}

static void obj_out_insn(struct instruction *i, struct obj_fdata *u, struct obj *o)
{
    o->records->data[u->insns] = i->u.word;

    u->insns++;

    obj_out_symbols(i->symbol, u, o);
    obj_out_reloc(i->reloc, u, o);
}

static int obj_out(FILE *stream, struct instruction *i, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;

    obj_out_insn(i, u, (struct obj*)u->o);

    return rc;
}

static int obj_fini(FILE *stream, void **ud)
{
    int rc = 0;

    struct obj_fdata *u = *ud;
    struct obj *o = u->o;

    if (u->flags & ASM_ASSEMBLE) {
        o->records->size = u->insns;
        o->sym_count = u->syms;
        o->rlc_count = u->rlcs;

        obj_write(u->o, stream);
    }

    obj_free(u->o);

    free(*ud);
    *ud = NULL;

    return rc;
}

/*******************************************************************************
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

/*******************************************************************************
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

