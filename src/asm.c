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
#include "param.h"
#include "os_common.h"

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

// aligned on 4 for speed
static const char arrows[4][4] = { "<-","->","<-","<-" };
static const char brackets[2][2] = { " ["," ]" };
enum { TYPE0=0, TYPE1, TYPE2, TYPE3, TYPEx };
enum { OP0=0, OP1, OP2, OP3, OPx };
static const int op_types[16] = {
    [OP_BITWISE_OR] = OP1,
    [OP_SUBTRACT  ] = OP2,
    [OP_ADD       ] = OP3,
    // all others   = OP0
};
// This table defines canonical shorthands for certain encodings
static const struct hides { unsigned a:1, b:1, c:1; }
hide[TYPEx][OPx] [2][2][2] = {
    [TYPE0][OP0] [0][0][1] = { 0,0,1 }, // B <- C * D
    [TYPE0][OP1] [0][0][1] = { 0,0,1 }, // B <- C | D
    [TYPE0][OP1] [1][0][0] = { 1,0,0 }, // B <-     D + 2
    [TYPE0][OP1] [1][0][1] = { 1,0,0 }, // B <-     D + 0
    [TYPE0][OP1] [1][1][0] = { 1,1,0 }, // B <-         2
    [TYPE0][OP1] [1][1][1] = { 1,1,0 }, // B <-         0
    [TYPE0][OP2] [0][0][1] = { 0,0,1 }, // B <- C - D
    [TYPE1][OP0] [0][0][1] = { 0,0,1 }, // B <- C * 2
    [TYPE1][OP0] [0][1][1] = { 0,0,1 }, // B <- C * 0
    [TYPE1][OP1] [0][0][1] = { 0,0,1 }, // B <- C | 2
    [TYPE1][OP1] [1][0][0] = { 1,0,0 }, // B <-     2 + D
    [TYPE1][OP1] [1][1][0] = { 1,1,0 }, // B <-         D
    [TYPE1][OP1] [1][1][1] = { 1,1,0 }, // B <-         A
    [TYPE2][OP0] [0][0][1] = { 0,0,1 }, // B <- 2 * C
    [TYPE2][OP0] [1][0][1] = { 0,0,1 }, // B <- 0 * C
    [TYPE2][OP1] [0][0][1] = { 0,0,1 }, // B <- 2 | C
    [TYPE2][OP1] [1][0][0] = { 1,0,0 }, // B <-     C + D
    [TYPE2][OP2] [0][0][1] = { 0,0,1 }, // B <- 2 - C
    [TYPE2][OP2] [1][0][1] = { 0,0,1 }, // B <- 0 - C
    [TYPE2][OP3] [1][0][1] = { 0,0,1 }, // B <- 0 + C
    [TYPE3][OP1] [1][1][0] = { 1,1,0 }, // B <-         2
};

static const int asm_op_loc[TYPEx][3] = {
    [TYPE0] = { 0, 1, 2 },
    [TYPE1] = { 1, 0, 2 },
    [TYPE2] = { 1, 2, 0 },
    [TYPE3] = { 0, 2, 2 },
};

static const char * const disasm_fmts[] = {
//         c0s1c2 s3 c4sA s6  sBsFsCc9   //
//  [0] = "%c%c%c %s %c"     "0"   "%c", // [Z] <- [      0     ]
    [1] = "%c%c%c %s %c%s"         "%c", // [Z] <- [X           ]
    [2] = "%c%c%c %s %c%s %3s %s"  "%c", // [Z] <- [X >>> Y     ]
    [3] = "%c%c%c %s %c%s %3s %s%s%s%c", // [Z] <- [X >>> Y + -0]
};

static int is_printable(unsigned int ch, char buf[3])
{
    buf[1] = buf[2] = '\0';

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

int print_disassembly(FILE *out, const struct element *i, int flags)
{
    if (flags & ASM_AS_DATA)
        return fprintf(out, ".word 0x%08x", i->insn.u.word);

    if (flags & ASM_AS_CHAR) {
        char buf[3];
        if (is_printable(i->insn.u.word, buf))
            return fprintf(out, ".word '%s'%*s", buf, buf[1] == '\0', "");
        else
            return fprintf(out, "          ");
    }

    const struct instruction_typeany * const g = &i->insn.u.typeany;
    const struct instruction_type012 * const t = &i->insn.u.type012;

    const int32_t imm   = g->p == 3 ? i->insn.u.type3.imm : i->insn.u.type012.imm;
    const int     width = g->p == 3 ? MEDIUM_IMMEDIATE_BITWIDTH : SMALL_IMMEDIATE_BITWIDTH;
    const int     op    = g->p == 3 ? OP_BITWISE_OR : t->op;

    const int fields[3] = { imm, t->y, t->x },
             *map       = asm_op_loc[g->p],
              lzero     = fields[map[2]] == 0,
              mzero     = fields[map[1]] == 0,
              rzero     = fields[map[0]] == 0;

    const struct hides hid = hide[g->p][op_types[op]][lzero][mzero][rzero];

    const int verbose = !!(flags & ASM_VERBOSE);

    const char    c0    = brackets[0][g->dd == 2]; // left side dereferenced ?
    const char    c1    = 'A' + g->z;              // register name for Z
    const char    c2    = brackets[1][g->dd == 2]; // left side dereferenced ?
    const char   *s3    = arrows[g->dd];           // arrow direction
    const char    c4    = brackets[0][g->dd & 1];  // right side dereferenced ?
    const char    s5[2] = { 'A' + g->x };          // register name for X
    const char   *s6    = op_names[op];            // operator name
    const char    s7[2] = { 'A' + t->y };          // register name for Y
    const int32_t i8    = SEXTEND32(width,imm);    // immediate value
    const char    c9    = brackets[1][g->dd & 1];  // right side dereferenced ?

    int show1 = (!hid.a || verbose) && g->p != 3,
        show2 = (!hid.b || verbose),
        show3 = (!hid.c || verbose),
        flip  = g->p == 0 && i8 < 0 && show2 && !verbose;

    char s8[16];
    const char *numfmt = (g->p == 3 || verbose) ? "0x%08x" : "%d";
    snprintf(s8, sizeof s8, numfmt, flip ? -i8 : i8);

    const char * const ss[] = { s8, s7, s5 },
               *sA   = ss[map[2]],
               *sB   = ss[map[1]],
               *sC   = ss[map[0]],
                sF[] = { ' ', "+-"[flip], ' ', '\0' }; // type0 sugar

    // Syntax sugars. type1 is not eligible (~0 and -0 become literals)
    if (lzero && !verbose && g->p != 1) {
        switch (op) {
            case OP_BITWISE_ORN:    // `B <- ~ C`
                s6 = "~";
                // FALLTHROUGH
            case OP_SUBTRACT:       // `B <- - C`
                sA = " ";
                show3 |= g->p == 0; // append term to disambiguate type0, type2
        }
    }

    // Centre a 1-to-3-character op
    char opstr[MAX_OP_LEN + 1];
    snprintf(opstr, sizeof opstr, "%-2s", s6);
    s6 = opstr;

    #define C_(C,B,A) (((C) << 2) | ((B) << 1) | (A))
    switch (C_(show1,show2,show3)) {
        #define PUT(...) \
            fprintf(out,disasm_fmts[show1 + show2 + show3],__VA_ARGS__)
        // Some combinations are never generated
    //  case C_(0,0,0): return PUT(c0,c1,c2,s3,c4,               c9);
        case C_(0,0,1): return PUT(c0,c1,c2,s3,c4,            sC,c9);
    //  case C_(0,1,0): return PUT(c0,c1,c2,s3,c4,      sB,      c9);
        case C_(0,1,1): return PUT(c0,c1,c2,s3,c4,sB,sF,      sC,c9);
    //  case C_(1,0,0): return PUT(c0,c1,c2,s3,c4,sA,            c9);
    //  case C_(1,0,1): return PUT(c0,c1,c2,s3,c4,sA,sF,      sC,c9);
        case C_(1,1,0): return PUT(c0,c1,c2,s3,c4,sA,s6,sB,      c9);
        case C_(1,1,1): return PUT(c0,c1,c2,s3,c4,sA,s6,sB,sF,sC,c9);
        #undef PUT

        default:
            fatal(0, "Unsupported show1,show2,show3 %d,%d,%d",
                    show1,show2,show3);
    }
    #undef C_
}

int print_registers(FILE *out, const int32_t regs[16])
{
    for (int i = 0; i < 15; i++) {
        fprintf(out, "%c %08x ", 'A' + i, regs[i]);
        if (i % 6 == 5)
            fputc('\n', out);
    }

    // Treat P specially : a read would return IP + 1
    fprintf(out, "%c %08x ", 'A' + 15, regs[15] + 1);

    fputc('\n', out);

    return 0;
}

int find_format_by_name(const void *_a, const void *_b)
{
    const struct format *a = _a, *b = _b;
    return strcmp(a->name, b->name);
}

/*******************************************************************************
 * general hooks
 */
static int gen_init(FILE *stream, struct param_state *p, void **ud)
{
    int *offset = *ud = malloc(sizeof *offset);
    *offset = 0;
    return 0;
}

static int gen_fini(FILE *stream, void **ud)
{
    free(*ud);
    *ud = NULL;
    return 0;
}

/*******************************************************************************
 * Object format : simple section-based objects
 */
struct obj_fdata {
    int assembling;
    struct obj *o;
    long syms;
    long rlcs;

    struct objrec *curr_rec;

    struct objsym **next_sym;
    struct objrlc **next_rlc;
    uint32_t pos;   ///< position in objrec

    int error;
};

static int obj_init(FILE *stream, struct param_state *p, void **ud)
{
    int rc = 0;

    struct obj_fdata *u = *ud = calloc(1, sizeof *u);
    struct obj *o = u->o = calloc(1, sizeof *o);

    param_get_int(p, "assembling", &u->assembling);

    if (u->assembling) {
        // TODO proper multiple-records support
        o->rec_count = 1;
        o->records = calloc(o->rec_count, sizeof *o->records);
        o->records[0].addr = 0;
        o->records[0].size = 1024; // will be realloc()ed as appropriate
        o->records[0].data = calloc(o->records->size, sizeof *o->records->data);
        u->curr_rec = o->records;

        u->next_sym = &o->symbols;
        u->next_rlc = &o->relocs;
    } else {
        rc = obj_read(u->o, stream);
        u->curr_rec = o->records;
    }

    return rc;
}

static int obj_in(FILE *stream, struct element *i, void *ud)
{
    struct obj_fdata *u = ud;

    struct objrec *rec = u->curr_rec;
    int done = 0;
    while (!done) {
        if (!rec) {
            u->error = 1;
            return -1;
        }

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

    return 1;
}

static void obj_out_insn(struct element *i, struct obj_fdata *u, struct obj *o)
{
    if (i->insn.size <= 0) {
        u->error = 1;
        return;
    }

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
    struct obj_fdata *u = ud;

    obj_out_insn(i, u, (struct obj*)u->o);

    return 1;
}

static int obj_sym(FILE *stream, struct symbol *symbol, int flags, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;

    if (symbol->global) {
        struct objsym *sym = *u->next_sym = calloc(1, sizeof *sym);

        strcopy(sym->name, symbol->name, sizeof sym->name);
        assert(("Symbol address resolved", symbol->resolved != 0));
        sym->value = symbol->reladdr;
        sym->flags = flags;

        u->next_sym = &sym->next;
        u->syms++;
    }

    return rc;
}

static int obj_reloc(FILE *stream, struct reloc_node *reloc, void *ud)
{
    struct obj_fdata *u = ud;
    if (!reloc || !reloc->insn) {
        u->error = 1;
        return 0;
    }

    struct objrlc *rlc = *u->next_rlc = calloc(1, sizeof *rlc);

    rlc->flags = reloc->flags;
    strcopy(rlc->name, reloc->name, sizeof rlc->name);
    rlc->name[sizeof rlc->name - 1] = 0;
    rlc->addr = reloc->insn->insn.reladdr;
    rlc->width = reloc->width;

    u->next_rlc = &rlc->next;

    u->rlcs++;

    return 1;
}

static int obj_emit(FILE *stream, void **ud)
{
    struct obj_fdata *u = *ud;
    struct obj *o = u->o;

    if (u->assembling) {
        o->records[0].size = u->pos;
        o->sym_count = u->syms;
        o->rlc_count = u->rlcs;

        obj_write(u->o, stream);
    }

    return 0;
}

static int obj_fini(FILE *stream, void **ud)
{
    struct obj_fdata *u = *ud;

    obj_free(u->o);

    free(*ud);
    *ud = NULL;

    return 0;
}

static int obj_err(void *ud)
{
    struct obj_fdata *u = ud;
    return !!u->error;
}

/*******************************************************************************
 * Raw format : raw binary data (host endian)
 */

static int raw_init(FILE *stream, struct param_state *p, void **ud)
{
    int rc = gen_init(stream, p, ud);
    os_set_binmode(stream);
    return rc;
}

static int raw_in(FILE *stream, struct element *i, void *ud)
{
    int *offset = ud;
    int rc = (fread(&i->insn.u.word, 4, 1, stream) == 1) ? 1 : -1;
    i->insn.u.word = fixup_endian(i->insn.u.word);
    i->insn.reladdr = (*offset)++;
    return rc;
}

static int raw_out(FILE *stream, struct element *i, void *ud)
{
    int *offset = ud;
    int ok = 1;
    static const int32_t zero = 0;
    while (ok && i->insn.reladdr > *offset) {
        if (fwrite(&zero, sizeof zero, 1, stream) != 1)
            return -1;
        ++*offset;
    }

    if (i->insn.size > 0) {
        UWord temp = fixup_endian(i->insn.u.word);
        if (fwrite(&temp, sizeof i->insn.u.word, 1, stream) != 1)
            return -1;
        ++*offset;
    }

    for (int c = 1; c < i->insn.size && ok; c++, ++*offset)
        if (fwrite(&zero, sizeof zero, 1, stream) != 1)
            return -1;

    return 1;
}

/*******************************************************************************
 * Text format : hexadecimal numbers
 */
static int text_init(FILE *stream, struct param_state *p, void **ud)
{
    gen_init(stream, p, ud);
    // This output might be consumed by a tool that needs a line at a time
    return setvbuf(stream, NULL, _IOLBF, 0);
}

static int text_in(FILE *stream, struct element *i, void *ud)
{
    int32_t *offset = ud;
    int result =
        fscanf(stream, "   %x", &i->insn.u.word) == 1 ||
        fscanf(stream, " 0x%x", &i->insn.u.word) == 1;
    i->insn.reladdr = (*offset)++;
    // Check for whitespace or EOF after the consumed item. This format can
    // read "agda"" as "0xa" and subsequently fail, when the whole string
    // should have been rejected.
    int next_char = fgetc(stream);
    if (next_char != EOF && !isspace(next_char))
        return -1;
    return result ? 1 : -1;
}

static int text_out(FILE *stream, struct element *i, void *ud)
{
    int ok = 1;
    if (i->insn.size > 0)
        ok &= fprintf(stream, "0x%08x\n", i->insn.u.word) > 0;
    for (int c = 1; c < i->insn.size && ok; c++)
        ok &= fputs("0x00000000\n", stream) > 0;
    return ok ? 1 : -1;
}

/*******************************************************************************
 * memh format : suitable for use with $readmemh() in Verilog
 */

struct memh_state {
    int32_t written, marked, offset;
    unsigned first_done:1;
    int emit_zeros;
    unsigned error:1;
};

static int memh_init(FILE *stream, struct param_state *p, void **ud)
{
    struct memh_state *state = *ud = calloc(1, sizeof *state);

    state->offset = 0;
    param_get_int(p, "format.memh.offset", &state->offset);
    state->marked = state->written = state->offset;

    param_get_int(p, "format.memh.explicit", &state->emit_zeros);

    return 0;
}

static int memh_in(FILE *stream, struct element *i, void *ud)
{
    struct memh_state *state = ud;

    if (state->marked > state->written) {
        i->insn.u.word = 0x0;
        i->insn.reladdr = state->written++;
        return 1;
    } else if (state->marked == state->written) {
        if (fscanf(stream, " @%x", (uint32_t*)&state->marked) == 1)
            return 0; // let next call handle it

        if (fscanf(stream, " %x", &i->insn.u.word) != 1)
            return -1;

        i->insn.reladdr = state->written++;
        state->marked = state->written;
        return 1;
    } else {
        state->error = 1;
        return -1; // @address moved backward (unsupported)
    }
}

static int memh_out(FILE *stream, struct element *i, void *ud)
{
    struct memh_state *state = ud;
    int32_t addr = i->insn.reladdr + state->offset;
    int32_t word = i->insn.u.word;
    int32_t diff = addr - state->written;

    if (word == 0 && !state->emit_zeros)
        return 0; // 0 indicates success but nothing was output

    state->written = addr;
    int printed = 0;
    if (diff > 1 || !state->first_done)
        printed = fprintf(stream, "@%x ", addr) > 2;

    state->first_done = 1;

    return (printed + fprintf(stream, "%08x\n", word) > 3) ? 1 : -1;
}

static int memh_err(void *ud)
{
    struct memh_state *state = ud;
    return !!state->error;
}

const struct format tenyr_asm_formats[] = {
    // first format is default
    { "obj",
        .init  = obj_init,
        .in    = obj_in,
        .out   = obj_out,
        .emit  = obj_emit,
        .fini  = obj_fini,
        .sym   = obj_sym,
        .reloc = obj_reloc,
        .err   = obj_err },
    { "raw",
        .init  = raw_init ,
        .in    = raw_in ,
        .out   = raw_out ,
        .fini  = gen_fini },
    { "text",
        .init  = text_init,
        .in    = text_in,
        .out   = text_out,
        .fini  = gen_fini },
    { "memh",
        .init  = memh_init,
        .in    = memh_in,
        .out   = memh_out,
        .fini  = gen_fini,
        .err   = memh_err },
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

int find_format(const char *optarg, const struct format **f)
{
    lfind_size_t sz = tenyr_asm_formats_count;
    *f = lfind(&(struct format){ .name = optarg }, tenyr_asm_formats, &sz,
            sizeof tenyr_asm_formats[0], find_format_by_name);
    return !*f;
}

/* vi: set ts=4 sw=4 et: */
