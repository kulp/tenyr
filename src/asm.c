#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "obj.h"
#include "ops.h"
#include "asm.h"
#include "common.h"
#include "parser_global.h"

static const struct {
    const char *name;
    int valid;
} op_meta[] = {
    [OP_ADD                ] = { "+" , 1 },
    [OP_SUBTRACT           ] = { "-" , 1 },
    [OP_MULTIPLY           ] = { "*" , 1 },

    [OP_COMPARE_LT         ] = { "<" , 1 },
    [OP_COMPARE_EQ         ] = { "==", 1 },
    [OP_COMPARE_GT         ] = { ">" , 1 },
    [OP_COMPARE_NE         ] = { "<>", 1 },
    [OP_BITWISE_OR         ] = { "|" , 1 },
    [OP_BITWISE_AND        ] = { "&" , 1 },
    [OP_BITWISE_ANDN       ] = { "&~", 1 },
    [OP_BITWISE_XOR        ] = { "^" , 1 },
    [OP_BITWISE_XORN       ] = { "^~", 1 },
    [OP_SHIFT_LEFT         ] = { "<<", 1 },
    [OP_SHIFT_RIGHT_LOGICAL] = { ">>", 1 },

    [OP_RESERVED0          ] = { "X0", 0 },
    [OP_RESERVED1          ] = { "X1", 0 },
};

static int is_printable(int ch, size_t len, char buf[len])
{
    memset(buf, 0, len);

    switch (ch) {
        case ' ' : buf[0] = ' ' ;                return 1;
        case '\\': buf[0] = '\\'; buf[1] = '\\'; return 1;
        case '\b': buf[0] = '\\'; buf[1] = 'b' ; return 1;
        case '\f': buf[0] = '\\'; buf[1] = 'f' ; return 1;
        case '\n': buf[0] = '\\'; buf[1] = 'n' ; return 1;
        case '\r': buf[0] = '\\'; buf[1] = 'r' ; return 1;
        case '\t': buf[0] = '\\'; buf[1] = 't' ; return 1;
        case '\v': buf[0] = '\\'; buf[1] = 'v' ; return 1;
        default: buf[0] = ch; return isprint((unsigned char)ch);
    }
}

int print_disassembly(FILE *out, struct instruction *i, int flags)
{
    if (flags & ASM_AS_DATA)
        return fprintf(out, ".word 0x%08x", i->u.word);

    if (flags & ASM_AS_CHAR) {
        char buf[10];
        if (is_printable(i->u.word, sizeof buf, buf))
            return fprintf(out, ".word '%s'%*s", buf, (int)(2 - strlen(buf)), "");
        else
            return fprintf(out, "          ");
    }

    int type = i->u._xxxx.t;
    switch (type) {
        default:
            if (i->u.word == 0xffffffff)
                return fprintf(out, "illegal");
            else
                return fprintf(out, ".word 0x%08x", i->u.word);

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
                  int32_t f0 = ld ? '[' : ' ';        // left side dereferenced ?
                  int32_t f1 = 'A' + g->z;            // register name for Z
                  int32_t f2 = ld ? ']' : ' ';        // left side dereferenced ?

            // arrow
            const char   *f3 = (g->dd == 3) ? "->" : "<-";    // arrow direction

            // RHS
                  int32_t f4 = rd ? '[' : ' ';        // right side dereferenced ?
                  int32_t f5 = 'A' + g->x;            // register name for X
            const char   *f6 = op_meta[g->op].name;   // operator name
                  int32_t f7 = 'A' + g->y;            // register name for Y
                  int32_t f8 = SEXTEND(12,g->imm);    // immediate value, sign-extended
                  int32_t f9 = rd ? ']' : ' ';        // right side dereferenced ?

            if (!op_meta[g->op].valid)   // reserved
                return fprintf(out, ".word 0x%08x", i->u.word);

            int inert = g->op == OP_BITWISE_OR || g->op == OP_ADD;
            int opXA  = g->x == 0;
            int opYA  = g->y == 0;
            int op3   = g->p ? !opYA : (!!g->imm);
            int op2   = !inert || (g->p ? g->imm : !opYA);
            int op1   = !(opXA && inert) || (!op2 && !op3);
            int kind  = g->p;

            // losslessly disambiguate these cases :
            //  b <- a
            //  b <- 0
            // so that assembly roundtripping works more reliably
            int rhs0  = (g->op == OP_ADD || (inert && kind == 1)) && opXA && !g->imm;
            int rhsA  = (g->op == OP_BITWISE_OR    && kind == 0)  && opXA && !g->imm;

            if (rhs0) {
                op3 = 1;
                op2 = 0;
                op1 = 0;
                kind = 0;
            } else if (rhsA) {
                op1 = 1;
                op2 = 0;
                op3 = 0;
            }

            if (!(flags & ASM_NO_SUGAR)) {
                if (g->op == OP_BITWISE_XORN && g->y == 0) {
                    f7 = f5;    // Y slot is now X
                    f5 = ' ';   // don't print X
                    f6 = "~";   // change op to a unary not
                } else if (g->op == OP_SUBTRACT && g->x == 0) {
                    f5 = ' ';   // don't bring X
                }
            }

            #define C_(D,C,B,A) (((D) << 3) | ((C) << 2) | ((B) << 1) | (A))
            // indices : g->p, op1, op2, op3
            static const char *fmts[] = {
                // args :       f0f1f2 f3 f4f5   f6 f7   f8   f9
              //[C_(0,0,0,0)] = "%c%c%c %s %c"                "%c", // [Z] <- [           ]
                [C_(0,0,0,1)] = "%c%c%c %s %c"           "%-10d%c", // [Z] <- [         -0]
                [C_(0,0,1,0)] = "%c%c%c %s %c"      "%c"      "%c", // [Z] <- [    Y      ]
                [C_(0,0,1,1)] = "%c%c%c %s %c"      "%c + %-10d%c", // [Z] <- [    Y +  -0]
                [C_(0,1,0,0)] = "%c%c%c %s %c%c"              "%c", // [Z] <- [X          ]
                [C_(0,1,0,1)] = "%c%c%c %s %c%c"      " + %-10d%c", // [Z] <- [X     +  -0]
                [C_(0,1,1,0)] = "%c%c%c %s %c%c %-2s %c"      "%c", // [Z] <- [X - Y      ]
                [C_(0,1,1,1)] = "%c%c%c %s %c%c %-2s %c + %-10d%c", // [Z] <- [X - Y +  -0]
                //args :        f0f1f2 f3 f4f5   f6 f8        f7f9
              //[C_(1,0,0,0)] = "%c%c%c %s %c"                "%c", // [Z] <- [           ]
                [C_(1,0,0,1)] = "%c%c%c %s %c"              "%c%c", // [Z] <- [          Y]
                [C_(1,0,1,0)] = "%c%c%c %s %c"     " %-10d"   "%c", // [Z] <- [     -0    ]
                [C_(1,0,1,1)] = "%c%c%c %s %c"     " %-10d + %c%c", // [Z] <- [     -0 + Y]
                [C_(1,1,0,0)] = "%c%c%c %s %c%c"              "%c", // [Z] <- [X          ]
                [C_(1,1,0,1)] = "%c%c%c %s %c%c"         " + %c%c", // [Z] <- [X       + Y]
                [C_(1,1,1,0)] = "%c%c%c %s %c%c %-2s %-10d"   "%c", // [Z] <- [X -  -0    ]
                [C_(1,1,1,1)] = "%c%c%c %s %c%c %-2s %-10d + %c%c", // [Z] <- [X -  -0 + Y]
            };

            #define PUT(...) return fprintf(out, fmts[C_(kind,op1,op2,op3)], __VA_ARGS__)

            if (kind == 1) {
                // swap f7 and f8 positions if this is a type1 instruction
                int32_t temp = f8;
                f8 = f7;
                f7 = temp;
            }

            switch (C_(0,op1,op2,op3)) {
              //case C_(0,0,0,0): PUT(f0,f1,f2,f3,f4,            f9); break;
                case C_(0,0,0,1): PUT(f0,f1,f2,f3,f4,         f8,f9); break;
                case C_(0,0,1,0): PUT(f0,f1,f2,f3,f4,      f7,   f9); break;
                case C_(0,0,1,1): PUT(f0,f1,f2,f3,f4,      f7,f8,f9); break;
                case C_(0,1,0,0): PUT(f0,f1,f2,f3,f4,f5,         f9); break;
                case C_(0,1,0,1): PUT(f0,f1,f2,f3,f4,f5,      f8,f9); break;
                case C_(0,1,1,0): PUT(f0,f1,f2,f3,f4,f5,f6,f7,   f9); break;
                case C_(0,1,1,1): PUT(f0,f1,f2,f3,f4,f5,f6,f7,f8,f9); break;

                default:
                    fatal(0, "Unsupported kind,op1,op2,op3 %05x",
                            C_(g->p,op1,op2,op3));
            }

            return 0;
        }
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
    int done = 0;
    while (!done) {
        if (!rec)
            return -1;

        if (rec->size == 0) {
            while (rec && rec->size == 0)
                u->curr_rec = rec = rec->next;
            u->pos = 0;
        } else {
            if (u->pos >= rec->size) {
                u->curr_rec = rec = rec->next;
                u->pos = 0;
            } else {
                i->u.word = rec->data[u->pos++];
                // TODO adjust addr where ?
                i->reladdr = rec->addr;
                i->symbol = NULL;
                done = 1;
            }
        }
    }

    return rc;
}

static void obj_out_insn(struct instruction *i, struct obj_fdata *u, struct obj *o)
{
    o->records->data[u->insns] = i->u.word;
    u->insns++;
}

static int obj_out(FILE *stream, struct instruction *i, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;

    obj_out_insn(i, u, (struct obj*)u->o);

    return rc;
}

static int obj_sym(FILE *stream, struct symbol *symbol, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;

    if (symbol->global) {
        struct objsym *sym = *u->next_sym = calloc(1, sizeof *sym);

        strcopy(sym->name, symbol->name, sizeof sym->name);
        assert(("Symbol address resolved", symbol->resolved != 0));
        sym->value = symbol->reladdr;

        u->next_sym = &sym->next;
        u->syms++;
    }

    return rc;
}

static int obj_reloc(FILE *stream, struct reloc_node *reloc, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;
    if (!reloc || !reloc->insn)
        return 0;

    struct objrlc *rlc = *u->next_rlc = calloc(1, sizeof *rlc);

    rlc->flags = reloc->flags;
    strcopy(rlc->name, reloc->name, sizeof rlc->name);
    rlc->name[sizeof rlc->name - 1] = 0;
    rlc->addr = reloc->insn->reladdr;
    rlc->width = reloc->width;

    u->next_rlc = &rlc->next;

    u->rlcs++;

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

/*******************************************************************************
 * Verilog format : behavioural assignment statements
 */
struct verilog_data {
    char storename[32];
};

static int verilog_init(FILE *stream, int flags, void **ud)
{
    int rc = 0;
    struct verilog_data *v = *ud = calloc(1, sizeof *v);

    // TODO make configurable
    snprintf(v->storename, sizeof v->storename, "store");

    return rc;
}

static int verilog_out(FILE *stream, struct instruction *i, void *ud)
{
    struct verilog_data *v = ud;
    return fprintf(stream, "%s[32'h%08x + BASE] = 32'h%08x;\n", v->storename, i->reladdr, i->u.word);
}

static int verilog_fini(FILE *stream, void **ud)
{
    int rc = 0;

    struct verilog_data *v = *ud;
    free(v);
    *ud = NULL;

    return rc;
}

const struct format formats[] = {
    // first format is default
    { "obj",
        .init  = obj_init,
        .in    = obj_in,
        .out   = obj_out,
        .fini  = obj_fini,
        .sym   = obj_sym,
        .reloc = obj_reloc },
    { "raw" , .in = raw_in , .out = raw_out  },
    { "text", .in = text_in, .out = text_out },
    { "verilog", .init = verilog_init, .out = verilog_out, .fini = verilog_fini },
};

const size_t formats_count = countof(formats);

int make_format_list(int (*pred)(const struct format *), size_t flen,
        const struct format fmts[flen], size_t len, char buf[len],
        const char *sep)
{
    int pos = 0;
    const struct format *f = fmts;
    while (pos < (signed)len && f < fmts + flen) {
        if (pred == NULL || pred(f)) {
            if (pos > 0) {
                pos += snprintf(&buf[pos], len - pos, "%s%s", sep, f->name);
            } else {
                pos += snprintf(&buf[pos], len - pos, "%s", f->name);
            }
        }

        f++;
    }

    return pos;
}

