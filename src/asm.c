#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "obj.h"
#include "ops.h"
#include "asm.h"
#include "common.h"
#include "parser_global.h"

// The length (excluding terminating NUL) of an operator length in disassembly
#define MAX_OP_LEN 3

static const struct {
    const char name[MAX_OP_LEN + 1];
    int hex;    ///< whether to use hex digits in disassembly
    int inertX; ///< if equivalent to `Z <- Y + I` when when X == 0
    int inertY; ///< if equivalent to `Z <- X + I` when when Y == 0
} op_meta[] = {
    [OP_ADD              ] = { "+"  , 0, 1, 1 },
    [OP_SUBTRACT         ] = { "-"  , 0, 0, 1 },
    [OP_MULTIPLY         ] = { "*"  , 0, 0, 0 },
    [OP_PACK             ] = { "^^" , 0, 1, 0 }, // pack is inert for X but not for Y
    [OP_COMPARE_LT       ] = { "<"  , 0, 0, 0 },
    [OP_COMPARE_EQ       ] = { "==" , 0, 0, 0 },
    [OP_COMPARE_GE       ] = { ">=" , 0, 0, 0 },
    [OP_COMPARE_NE       ] = { "<>" , 0, 0, 0 },
    [OP_BITWISE_OR       ] = { "|"  , 1, 1, 1 },
    [OP_BITWISE_ORN      ] = { "|~" , 1, 0, 0 },
    [OP_BITWISE_AND      ] = { "&"  , 1, 0, 0 },
    [OP_BITWISE_ANDN     ] = { "&~" , 1, 0, 1 },
    [OP_BITWISE_XOR      ] = { "^"  , 1, 1, 1 },
    [OP_SHIFT_LEFT       ] = { "<<" , 0, 0, 1 },
    [OP_SHIFT_RIGHT_LOGIC] = { ">>" , 0, 0, 1 },
    [OP_SHIFT_RIGHT_ARITH] = { ">>>", 0, 0, 1 },
};

static int is_printable(unsigned int ch, size_t len, char buf[len])
{
    memset(buf, 0, len);

    switch (ch) {
        case ' ' : buf[0] = ' ' ;                return 1;
        case '\\': buf[0] = '\\'; buf[1] = '\\'; return 1;
        case '\0': buf[0] = '\\'; buf[1] = '0' ; return 1;
        case '\b': buf[0] = '\\'; buf[1] = 'b' ; return 1;
        case '\f': buf[0] = '\\'; buf[1] = 'f' ; return 1;
        case '\n': buf[0] = '\\'; buf[1] = 'n' ; return 1;
        case '\r': buf[0] = '\\'; buf[1] = 'r' ; return 1;
        case '\t': buf[0] = '\\'; buf[1] = 't' ; return 1;
        case '\v': buf[0] = '\\'; buf[1] = 'v' ; return 1;
        default: buf[0] = ch; return ch < UCHAR_MAX && isprint((unsigned char)ch);
    }
}

int print_disassembly(FILE *out, struct element *i, int flags)
{
    if (flags & ASM_AS_DATA)
        return fprintf(out, ".word 0x%08x", i->insn.u.word);

    if (flags & ASM_AS_CHAR) {
        char buf[10];
        if (is_printable(i->insn.u.word, sizeof buf, buf))
            return fprintf(out, ".word '%s'%*s", buf, (int)(2 - strlen(buf)), "");
        else
            return fprintf(out, "          ");
    }

    struct instruction_typeany *g = &i->insn.u.typeany;
    struct instruction_type012 *t = &i->insn.u.type012;
    struct instruction_type3   *v = &i->insn.u.type3;

    int rd = g->dd &  1;
    int ld = g->dd == 2;

    int width;
    int32_t imm;
    switch (g->p) {
        case 0:
        case 1:
        case 2:
            width = SMALL_IMMEDIATE_BITWIDTH;
            imm = t->imm;
            break;
        case 3:
            width = MEDIUM_IMMEDIATE_BITWIDTH;
            imm = v->imm;
            break;
    }

    // LHS
          char    f0 = ld ? '[' : ' ';        // left side dereferenced ?
          char    f1 = 'A' + g->z;            // register name for Z
          char    f2 = ld ? ']' : ' ';        // left side dereferenced ?

    // arrow
    const char   *f3 = (g->dd == 3) ? "->" : "<-";    // arrow direction

    // RHS
          char    f4 = rd ? '[' : ' ';        // right side dereferenced ?
          char    f5 = 'A' + g->x;            // register name for X
    const char   *f6 = op_meta[t->op].name;   // operator name
          char    f7 = 'A' + t->y;            // register name for Y
          int32_t f8 = SEXTEND32(width,imm);  // immediate value, sign-extended
          char    f9 = rd ? ']' : ' ';        // right side dereferenced ?

    int hex   = op_meta[t->op].hex;
    int opX0  = g->x == 0 || (imm == 0 && g->p == 2);
    int opY0  = t->y == 0 || (imm == 0 && g->p == 1);
    int inert = (op_meta[t->op].inertX && opX0) || (op_meta[t->op].inertY && opY0);
    int op3   = g->p ? !opY0 : (!!imm);
    int op2   = (!inert || (g->p ? (imm || opY0) : !opY0));
    int op1   = !(opX0 && inert) || (!op2 && !op3);

    if (flags & ASM_VERBOSE)
        op1 = op2 = op3 = hex = 1;

    if (g->p == 3) {
        hex = op1 = 1; // Large immediates are always emitted as hex
        op2 = op3 = 0;
    } else if (!(flags & (ASM_NO_SUGAR | ASM_VERBOSE))) {
        if (t->op == OP_BITWISE_ORN && t->x == 0) {
            f5 = ' ';   // don't print X
            f6 = "~";   // change op to a unary not
        } else if (t->op == OP_SUBTRACT && g->x == 0) {
            f5 = ' ';   // don't print X
        }

        // sugar for P update idiom ; prefer signed decimal operand
        if (t->y == 15
            && (    (t->op == OP_BITWISE_AND)
                 || (t->op == OP_BITWISE_OR && g->x == 0)
                 || (t->op == OP_ADD)
                 || (t->op == OP_SUBTRACT)
               )
           ) {
            hex = 0;
        }
    }

    #define C_(E,D,C,B,A) (((E) << 3) | ((D) << 5) | ((C) << 2) | ((B) << 1) | (A))
    // We'd like to use positional references in these format strings, but to
    // preserve compatibility with Win32 libc, we don't.
    // indices : hex, g->p, op1, op2, op3
    static const char *fmts[C_(1,3,1,1,1)+1] = {
      //[C_(0,0,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(0,0,0,0,1)] = "%c%c%c %s %c"                "%d%c", // [Z] <- [          -0]
        [C_(0,0,0,1,0)] = "%c%c%c %s %c"        "%c"      "%c", // [Z] <- [     Y      ]
        [C_(0,0,0,1,1)] = "%c%c%c %s %c"        "%c + " "%d%c", // [Z] <- [     Y +  -0]
        [C_(0,0,1,0,0)] = "%c%c%c %s %c%c"                "%c", // [Z] <- [ X          ]
        [C_(0,0,1,0,1)] = "%c%c%c %s %c%c"        " + " "%d%c", // [Z] <- [ X     +  -0]
        [C_(0,0,1,1,0)] = "%c%c%c %s %c%c %3s " "%c"      "%c", // [Z] <- [ X - Y      ]
        [C_(0,0,1,1,1)] = "%c%c%c %s %c%c %3s " "%c + " "%d%c", // [Z] <- [ X - Y +  -0]
      //[C_(0,1,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(0,1,0,0,1)] = "%c%c%c %s %c"                "%c%c", // [Z] <- [           Y]
        [C_(0,1,0,1,0)] = "%c%c%c %s %c"           "%d"   "%c", // [Z] <- [      -0    ]
        [C_(0,1,0,1,1)] = "%c%c%c %s %c"           "%d + %c%c", // [Z] <- [      -0 + Y]
        [C_(0,1,1,0,0)] = "%c%c%c %s %c%c"                "%c", // [Z] <- [ X          ]
        [C_(0,1,1,0,1)] = "%c%c%c %s %c%c"           " + %c%c", // [Z] <- [ X       + Y]
        [C_(0,1,1,1,0)] = "%c%c%c %s %c%c %3s "    "%d"   "%c", // [Z] <- [ X -  -0    ]
        [C_(0,1,1,1,1)] = "%c%c%c %s %c%c %3s "    "%d + %c%c", // [Z] <- [ X -  -0 + Y]
      //[C_(0,2,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(0,2,0,0,1)] = "%c%c%c %s %c"                "%c%c", // [Z] <- [           Y]
        [C_(0,2,0,1,0)] = "%c%c%c %s %c"        "%c"      "%c", // [Z] <- [     X      ]
        [C_(0,2,0,1,1)] = "%c%c%c %s %c"        "%c + " "%c%c", // [Z] <- [     X +   Y]
        [C_(0,2,1,0,0)] = "%c%c%c %s %c%d"                "%c", // [Z] <- [-0          ]
        [C_(0,2,1,0,1)] = "%c%c%c %s %c%d"        " + " "%c%c", // [Z] <- [-0     +   Y]
        [C_(0,2,1,1,0)] = "%c%c%c %s %c%d %3s " "%c"      "%c", // [Z] <- [-0 - X      ]
        [C_(0,2,1,1,1)] = "%c%c%c %s %c%d %3s " "%c + " "%c%c", // [Z] <- [-0 - X +   Y]
      //[C_(0,3,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(0,3,1,0,0)] = "%c%c%c %s %c%d"                "%c", // [Z] <- [-0          ]
      //[C_(1,0,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(1,0,0,0,1)] = "%c%c%c %s %c"            "0x%08x%c", // [Z] <- [         0x0]
        [C_(1,0,0,1,0)] = "%c%c%c %s %c"       "%c"       "%c", // [Z] <- [     Y      ]
        [C_(1,0,0,1,1)] = "%c%c%c %s %c"       "%c + 0x%08x%c", // [Z] <- [     Y + 0x0]
        [C_(1,0,1,0,0)] = "%c%c%c %s %c%c"                "%c", // [Z] <- [ X          ]
        [C_(1,0,1,0,1)] = "%c%c%c %s %c%c"       " + 0x%08x%c", // [Z] <- [ X     + 0x0]
        [C_(1,0,1,1,0)] = "%c%c%c %s %c%c %3s ""%c"       "%c", // [Z] <- [ X - Y      ]
        [C_(1,0,1,1,1)] = "%c%c%c %s %c%c %3s ""%c + 0x%08x%c", // [Z] <- [ X - Y + 0x0]
      //[C_(1,1,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(1,1,0,0,1)] = "%c%c%c %s %c"                "%c%c", // [Z] <- [           Y]
        [C_(1,1,0,1,0)] = "%c%c%c %s %c"       "0x%08x"   "%c", // [Z] <- [     0x0    ]
        [C_(1,1,0,1,1)] = "%c%c%c %s %c"       "0x%08x + %c%c", // [Z] <- [     0x0 + Y]
        [C_(1,1,1,0,0)] = "%c%c%c %s %c%c"                "%c", // [Z] <- [ X          ]
        [C_(1,1,1,0,1)] = "%c%c%c %s %c%c"           " + %c%c", // [Z] <- [ X       + Y]
        [C_(1,1,1,1,0)] = "%c%c%c %s %c%c %3s ""0x%08x"   "%c", // [Z] <- [ X - 0x0    ]
        [C_(1,1,1,1,1)] = "%c%c%c %s %c%c %3s ""0x%08x + %c%c", // [Z] <- [ X - 0x0 + Y]
      //[C_(1,0,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(1,2,0,0,1)] = "%c%c%c %s %c"                "%c%c", // [Z] <- [           Y]
        [C_(1,2,0,1,0)] = "%c%c%c %s %c"       "%c"       "%c", // [Z] <- [       X    ]
        [C_(1,2,0,1,1)] = "%c%c%c %s %c"       "%c + "  "%c%c", // [Z] <- [       X + Y]
        [C_(1,2,1,0,0)] = "%c%c%c %s %c0x%08x"            "%c", // [Z] <- [0x0         ]
        [C_(1,2,1,0,1)] = "%c%c%c %s %c0x%08x"       " + %c%c", // [Z] <- [0x0      + Y]
        [C_(1,2,1,1,0)] = "%c%c%c %s %c0x%08x %3s ""%c"   "%c", // [Z] <- [0x0 -  X    ]
        [C_(1,2,1,1,1)] = "%c%c%c %s %c0x%08x %3s ""%c + %c%c", // [Z] <- [0x0 -  X + Y]
      //[C_(1,3,0,0,0)] = "%c%c%c %s %c"                  "%c", // [Z] <- [            ]
        [C_(1,3,1,0,0)] = "%c%c%c %s %c0x%08x"            "%c", // [Z] <- [0x0         ]
    };

    // Centre a 1-to-3-character op
    char op[MAX_OP_LEN + 1];
    snprintf(op, sizeof op, "%-2s", f6);
    f6 = op;

    #define PUT(...) return fprintf(out, fmts[C_(hex,g->p,op1,op2,op3)], __VA_ARGS__)
    switch (C_(0,g->p,op1,op2,op3)) {
      //case C_(0,0,0,0,0): PUT(f0,f1,f2,f3,f4,            f9); break;
        case C_(0,0,0,0,1): PUT(f0,f1,f2,f3,f4,         f8,f9); break;
        case C_(0,0,0,1,0): PUT(f0,f1,f2,f3,f4,      f7,   f9); break;
        case C_(0,0,0,1,1): PUT(f0,f1,f2,f3,f4,      f7,f8,f9); break;
        case C_(0,0,1,0,0): PUT(f0,f1,f2,f3,f4,f5,         f9); break;
        case C_(0,0,1,0,1): PUT(f0,f1,f2,f3,f4,f5,      f8,f9); break;
        case C_(0,0,1,1,0): PUT(f0,f1,f2,f3,f4,f5,f6,f7,   f9); break;
        case C_(0,0,1,1,1): PUT(f0,f1,f2,f3,f4,f5,f6,f7,f8,f9); break;
      //case C_(0,1,0,0,0): PUT(f0,f1,f2,f3,f4,            f9); break;
        case C_(0,1,0,0,1): PUT(f0,f1,f2,f3,f4,         f7,f9); break;
        case C_(0,1,0,1,0): PUT(f0,f1,f2,f3,f4,      f8,   f9); break;
        case C_(0,1,0,1,1): PUT(f0,f1,f2,f3,f4,      f8,f7,f9); break;
        case C_(0,1,1,0,0): PUT(f0,f1,f2,f3,f4,f5,         f9); break;
        case C_(0,1,1,0,1): PUT(f0,f1,f2,f3,f4,f5,      f7,f9); break;
        case C_(0,1,1,1,0): PUT(f0,f1,f2,f3,f4,f5,f6,f8,   f9); break;
        case C_(0,1,1,1,1): PUT(f0,f1,f2,f3,f4,f5,f6,f8,f7,f9); break;
      //case C_(0,2,0,0,0): PUT(f0,f1,f2,f3,f4,            f9); break;
        case C_(0,2,0,0,1): PUT(f0,f1,f2,f3,f4,         f7,f9); break;
        case C_(0,2,0,1,0): PUT(f0,f1,f2,f3,f4,      f5,   f9); break;
        case C_(0,2,0,1,1): PUT(f0,f1,f2,f3,f4,      f5,f7,f9); break;
        case C_(0,2,1,0,0): PUT(f0,f1,f2,f3,f4,f8,         f9); break;
        case C_(0,2,1,0,1): PUT(f0,f1,f2,f3,f4,f8,      f7,f9); break;
        case C_(0,2,1,1,0): PUT(f0,f1,f2,f3,f4,f8,f6,f5,   f9); break;
        case C_(0,2,1,1,1): PUT(f0,f1,f2,f3,f4,f8,f6,f5,f7,f9); break;
      //case C_(0,3,0,0,0): PUT(f0,f1,f2,f3,f4,            f9); break;
        case C_(0,3,1,0,0): PUT(f0,f1,f2,f3,f4,f8,         f9); break;

        default:
            fatal(0, "Unsupported hex,kind,op1,op2,op3 %d,%d,%d,%d,%d",
                    hex,g->p,op1,op2,op3);
    }
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

static int obj_in(FILE *stream, struct element *i, void *ud)
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
                i->insn.u.word = rec->data[u->pos++];
                // TODO adjust addr where ?
                i->insn.reladdr = rec->addr;
                i->symbol = NULL;
                done = 1;
            }
        }
    }

    return rc;
}

static void obj_out_insn(struct element *i, struct obj_fdata *u, struct obj *o)
{
    // TODO handle i->insn.size > 1 better. It should store .zero data
    // sparsely.
    o->records->data[u->insns++] = i->insn.u.word;
    for (size_t c = 1; c < i->insn.size; c++)
        o->records->data[u->insns++] = 0;
}

static int obj_out(FILE *stream, struct element *i, void *ud)
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
    rlc->addr = reloc->insn->insn.reladdr;
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
static int raw_in(FILE *stream, struct element *i, void *ud)
{
    return fread(&i->insn.u.word, 4, 1, stream) == 1;
}

static int raw_out(FILE *stream, struct element *i, void *ud)
{
    int ok = 1;
    ok &= fwrite(&i->insn.u.word, sizeof i->insn.u.word, 1, stream) == 1;
    for (size_t c = 1; c < i->insn.size && ok; c++)
        ok &= fputc(0, stream) == 0;
    return ok;
}

/*******************************************************************************
 * Text format : hexadecimal numbers
 */
static int text_init(FILE *stream, int flags, void **ud)
{
    // This output might be consumed by a tool that needs a line at a time
    return setvbuf(stream, NULL, _IOLBF, 0);
}

static int text_in(FILE *stream, struct element *i, void *ud)
{
    int result =
        fscanf(stream, "   %x", &i->insn.u.word) == 1 ||
        fscanf(stream, " 0x%x", &i->insn.u.word) == 1;
    // Check for whitespace or EOF after the consumed item. This format can
    // read "agda"" as "0xa" and subsequently fail, when the whole string
    // should have been rejected.
    int next_char = fgetc(stream);
    if (!isspace(next_char) && next_char != EOF)
        return 0;
    return result;
}

static int text_out(FILE *stream, struct element *i, void *ud)
{
    int ok = 1;
    ok &= fprintf(stream, "0x%08x\n", i->insn.u.word) > 0;
    for (size_t c = 1; c < i->insn.size && ok; c++)
        ok &= fputs("0x00000000\n", stream) > 0;
    return ok;
}

/*******************************************************************************
 * memh format : suitable for use with $readmemh() in Verilog
 */
static int memh_init(FILE *stream, int flags, void **ud)
{
    // Use ud as a "last-address-written" marker. Only write addresses
    // explicitly when gaps appear. This has not been tested enough, and will
    // probably require more finesse when gaps do appear, when memh images are
    // loaded into ram.v's OFFSET RAMs.
    int *last = *ud = malloc(sizeof *last);
    *last = -1;

    return 0;
}

static int memh_fini(FILE *stream, void **ud)
{
    free(*ud);
    *ud = NULL;
    return 0;
}

static int memh_out(FILE *stream, struct element *i, void *ud)
{
    int *last = ud;
    int diff = i->insn.reladdr - *last;
    *last = i->insn.reladdr;
    return diff > 1
        ? (fprintf(stream, "@%x %08x\n", i->insn.reladdr, i->insn.u.word) > 0)
        : (fprintf(stream,     "%08x\n",                  i->insn.u.word) > 0);
}

const struct format tenyr_asm_formats[] = {
    // first format is default
    { "obj",
        .init  = obj_init,
        .in    = obj_in,
        .out   = obj_out,
        .fini  = obj_fini,
        .sym   = obj_sym,
        .reloc = obj_reloc },
    { "raw" , .in = raw_in , .out = raw_out  },
    { "text", .init = text_init, .in = text_in, .out = text_out },
    { "memh", .init = memh_init, .out = memh_out, .fini = memh_fini },
};

const size_t tenyr_asm_formats_count = countof(tenyr_asm_formats);

int make_format_list(int (*pred)(const struct format *), size_t flen,
        const struct format fmts[flen], size_t len, char buf[len],
        const char *sep)
{
    size_t pos = 0;
    for (const struct format *f = fmts; pos < len && f < fmts + flen; f++)
        if (!pred || pred(f))
            pos += snprintf(&buf[pos], len - pos, "%s%s", pos ? sep : "", f->name);

    return pos;
}

/* vi: set ts=4 sw=4 et: */
