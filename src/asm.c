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

static const char op_names[][MAX_OP_LEN + 1] = {
    [OP_ADD              ] = "+",
    [OP_SUBTRACT         ] = "-",
    [OP_MULTIPLY         ] = "*",
    [OP_PACK             ] = "^^",
    [OP_TEST_BIT         ] = "@",
    [OP_COMPARE_LT       ] = "<",
    [OP_COMPARE_EQ       ] = "==",
    [OP_COMPARE_GE       ] = ">=",
    [OP_BITWISE_OR       ] = "|",
    [OP_BITWISE_ORN      ] = "|~",
    [OP_BITWISE_AND      ] = "&",
    [OP_BITWISE_ANDN     ] = "&~",
    [OP_BITWISE_XOR      ] = "^",
    [OP_SHIFT_LEFT       ] = "<<",
    [OP_SHIFT_RIGHT_ARITH] = ">>",
    [OP_SHIFT_RIGHT_LOGIC] = ">>>",
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

    int width = SMALL_IMMEDIATE_BITWIDTH;
    int32_t imm = t->imm;
    if (g->p == 3) {
        width = MEDIUM_IMMEDIATE_BITWIDTH;
        imm = v->imm;
    }

    static const char regs[16 * 2] =
        "A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0P\0";
    static const char arrows[4 * 4] =
        "<-\0\0<-\0\0<-\0\0->\0\0";
    static const char brackets[2][2] =
        { " [", " ]" };

    const char    c0 = brackets[0][g->dd == 2]; // left side dereferenced ?
    const char   *s1 = &regs[g->z * 2];         // register name for Z
    const char    c2 = brackets[1][g->dd == 2]; // left side dereferenced ?
    const char   *s3 = &arrows[g->dd * 4];      // arrow direction
    const char    c4 = brackets[0][g->dd & 1];  // right side dereferenced ?
    const char   *s5 = &regs[g->x * 2];         // register name for X
    const char   *s6 = op_names[t->op];         // operator name
    const char   *s7 = &regs[t->y * 2];         // register name for Y
    const int32_t i8 = SEXTEND32(width,imm);    // immediate value
    const char    c9 = brackets[1][g->dd & 1];  // right side dereferenced ?

    int hex   = g->p == 3;
    int mid   = g->p == 0 ? t->y :
                g->p == 2 ? t->x : 0;
    int show1 = ((g->p >  1) ? imm != 0 : t->x != 0) || t->op != OP_BITWISE_OR;
    int show2 = ((g->p == 1) ? imm != 0 : mid  != 0) || t->op != OP_BITWISE_OR;
    int show3 = ((g->p == 0) ? imm != 0 : t->y != 0);

    if (flags & ASM_VERBOSE)
        show1 = show2 = show3 = hex = 1;

    char s8[16];
    snprintf(s8, sizeof s8, hex ? "0x%08x" : "%d", i8);

    const char *sA = NULL, *sB = NULL, *sC = NULL;
    switch (g->p) {
        case 0: sA = s5, sB = s7, sC = s8; break;
        case 1: sA = s5, sB = s8, sC = s7; break;
        case 2: sA = s8, sB = s5, sC = s7; break;
        case 3: sA = s8; show1 = 1; show2 = show3 = 0; break;
    }

    // Edge cases : show more operands if the instruction type can't be inferred
    if (show1 + show2 + show3 < 3) {
        switch (g->p) {
            case 0:
                if (t->op == OP_SUBTRACT && t->x == 0)
                    sA = " ", show3 = 1; // sugar for `B <- - C + 0`
                else if (t->op == OP_BITWISE_ORN && t->x == 0)
                    s6 = "~", sA = " ", show3 = 1; // sugar for `B <- ~ C + 0`
                else if (t->op == OP_BITWISE_OR && (t->x ? t->y == 0 && imm != 0 : t->y))
                    show1 = show2 = show3 = 1;
                break;
            case 1:
                if (t->op != OP_BITWISE_OR && t->y == 0)
                    show1 = show2 = 1;
                else if (t->x == 0 || t->y != 0 || t->op == OP_BITWISE_OR)
                    show1 = show2 = show3 = 1;
                break;
            case 2:
                if (t->op == OP_SUBTRACT && imm == 0)
                    sA = " "; // sugar for `B <- - C`
                else if (t->op == OP_BITWISE_ORN && imm == 0)
                    s6 = "~", sA = " "; // sugar for `B <- ~ C`
                else if (t->op != OP_BITWISE_OR && t->y == 0)
                    show1 = show2 = 1;
                else if (imm == 0 || (t->op == OP_BITWISE_OR && t->x == 0))
                    show1 = show2 = show3 = 1;
                break;
        }
    }

    // Edge case : all operands are 0, but word may not be 0
    if (g->p != 3 && !(show1 | show2 | show3))
        show1 = show2 = show3 = g->p != 0;

    static const char *fmts[] = {
    //   c0s1c2 s3 c4sA s6  sB   sCc9   //
        "%c%s%c %s %c"     "0"    "%c", // [Z] <- [      0     ]
        "%c%s%c %s %c%s"          "%c", // [Z] <- [X           ]
        "%c%s%c %s %c%s %3s %s"   "%c", // [Z] <- [X >>> Y     ]
        "%c%s%c %s %c%s %3s %s + %s%c", // [Z] <- [X >>> Y + -0]
    };

    {
        // Centre a 1-to-3-character op
        char op[MAX_OP_LEN + 1];
        snprintf(op, sizeof op, "%-2s", s6);
        s6 = op;
    }

    #define C_(C,B,A) (((C) << 2) | ((B) << 1) | (A))
    #define PUT(...) return fprintf(out, fmts[show1+show2+show3], __VA_ARGS__)
    switch (C_(show1,show2,show3)) {
        case C_(0,0,0): PUT(c0,s1,c2,s3,c4,            c9); break;
        case C_(0,0,1): PUT(c0,s1,c2,s3,c4,         sC,c9); break;
        case C_(0,1,0): PUT(c0,s1,c2,s3,c4,      sB,   c9); break;
        case C_(0,1,1): PUT(c0,s1,c2,s3,c4,sB," + ",sC,c9); break;
        case C_(1,0,0): PUT(c0,s1,c2,s3,c4,sA,         c9); break;
        case C_(1,0,1): PUT(c0,s1,c2,s3,c4,sA," + ",sC,c9); break;
        case C_(1,1,0): PUT(c0,s1,c2,s3,c4,sA,s6,sB,   c9); break;
        case C_(1,1,1): PUT(c0,s1,c2,s3,c4,sA,s6,sB,sC,c9); break;

        default:
            fatal(0, "Unsupported hex,kind,show1,show2,show3 %d,%d,%d,%d,%d",
                    hex,g->p,show1,show2,show3);
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

    for (; i < 15; i++)
        fprintf(out, "%c %08x ", 'A' + i, regs[i]);

    // Treat P specially : a read would return IP + 1
    fprintf(out, "%c %08x ", 'A' + 15, regs[15] + 1);

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
        o->records[0].addr = 0;
        o->records[0].size = 1024; // will be realloc()ed as appropriate
        o->records[0].data = calloc(o->records->size, sizeof *o->records->data);
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
                i->insn.reladdr = rec->addr + u->pos;
                i->insn.u.word = rec->data[u->pos++];
                i->symbol = NULL;
                done = 1;
            }
        }
    }

    return rc;
}

static void obj_out_insn(struct element *i, struct obj_fdata *u, struct obj *o)
{
    if (i->insn.size <= 0)
        return;

    struct objrec *rec = &o->records[0];
    if (rec->size < u->pos + i->insn.size) {
        // TODO rewrite this without realloc() (start a new section ?)
        while (rec->size < u->pos + i->insn.size)
            rec->size *= 2;
        rec->data = realloc(rec->data, rec->size * sizeof *rec->data);
    }

    // We could store .zero data sparsely, but we don't (yet)
    memset(&rec->data[u->pos], 0x00, i->insn.size);
    rec->data[u->pos] = i->insn.u.word;
    u->pos += i->insn.size;
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
        o->records[0].size = u->pos;
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
    return (fread(&i->insn.u.word, 4, 1, stream) == 1) ? 1 : -1;
}

static int raw_out(FILE *stream, struct element *i, void *ud)
{
    int ok = 1;
    ok &= fwrite(&i->insn.u.word, sizeof i->insn.u.word, 1, stream) == 1;

    const int32_t zero = 0;
    for (int c = 1; c < i->insn.size && ok; c++)
        ok &= fwrite(&zero, sizeof zero, 1, stream) == 1;

    return ok ? 1 : -1;
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
        return -1;
    return result ? 1 : -1;
}

static int text_out(FILE *stream, struct element *i, void *ud)
{
    int ok = 1;
    ok &= fprintf(stream, "0x%08x\n", i->insn.u.word) > 0;
    for (int c = 1; c < i->insn.size && ok; c++)
        ok &= fputs("0x00000000\n", stream) > 0;
    return ok ? 1 : -1;
}

/*******************************************************************************
 * memh format : suitable for use with $readmemh() in Verilog
 */
static int memh_init(FILE *stream, int flags, void **ud)
{
    // Track last-written (last[0]) and last-marked (last[1])
    int32_t *last = *ud = malloc(2 * sizeof *last);
    last[0] = 0;
    last[1] = 0;

    return 0;
}

static int memh_fini(FILE *stream, void **ud)
{
    free(*ud);
    *ud = NULL;
    return 0;
}

static int memh_in(FILE *stream, struct element *i, void *ud)
{
    int32_t *last = ud;

    if (last[1] > last[0]) {
        i->insn.u.word = 0x0;
        i->insn.reladdr = last[0]++;
        return 1;
    } else if (last[0] == last[1]) {
        if (fscanf(stream, " @%x", (uint32_t*)&last[1]) == 1)
            return 0; // let next call handle it

        if (fscanf(stream, " %x", &i->insn.u.word) != 1)
            return -1;

        i->insn.reladdr = last[0]++;
        last[1] = last[0];
        return 1;
    } else {
        return -1; // @address moved backward (unsupported)
    }
}

static int memh_out(FILE *stream, struct element *i, void *ud)
{
    int32_t *last = ud;
    int32_t addr = i->insn.reladdr;
    int32_t word = i->insn.u.word;
    int32_t diff = addr - last[0];

    if (word == 0)
        return 0; // 0 indicates success but nothing was output

    last[0] = addr;
    if (diff > 1)
        return (fprintf(stream, "@%x %08x\n", addr, word) > 0) ? 1 : -1;
    else
        return (fprintf(stream,     "%08x\n",       word) > 0) ? 1 : -1;
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
    { "memh", .init = memh_init, .in = memh_in, .out = memh_out, .fini = memh_fini },
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
