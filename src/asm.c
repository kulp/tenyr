#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "obj.h"
#include "ops.h"
#include "asm.h"
#include "common.h"
#include "parser_global.h"
#include "param.h"
#include "os_common.h"
#include "stream.h"

// The length (excluding terminating NUL) of an operator length in disassembly
#define MAX_OP_LEN 3

// Duplicate a string's contents, rounding up the length the a multiple of the
// length of an SWord, plus a space for the trailing '\0', with the unused
// bytes set to 0.
static inline char *strdup_rounded_up(char *in)
{
    size_t len = round_up_to_word(strlen(in));
    char *out = malloc(len + 1);
    return strncpy(out, in, len + 1);
}

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
static const struct hides { unsigned char a:1, b:1, c:1, :(CHAR_BIT-3); }
hide[TYPEx][OPx] [2][2][2] = {
    [TYPE0][OP0] [0][0][1] = { 0,0,1 }, // B <- C * D
    [TYPE0][OP1] [0][0][1] = { 0,0,1 }, // B <- C | D
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
    [TYPE3][OP1] [1][1][1] = { 1,1,0 }, // B <-         0
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
        default:
            buf[0] = (char)ch;
            return ch < UCHAR_MAX && isprint((unsigned char)ch);
    }
}

int print_disassembly(STREAM *out, const struct element *i, int flags)
{
    if (flags & ASM_AS_DATA)
        return out->op.fprintf(out, ".word 0x%08x", i->insn.u.word);

    if (flags & ASM_AS_CHAR) {
        char buf[3];
        if (is_printable((unsigned)i->insn.u.word, buf))
            return out->op.fprintf(out, ".word '%s'%*s", buf, buf[1] == '\0', "");
        else
            return out->op.fprintf(out, "          ");
    }

    const struct instruction_typeany * const g = &i->insn.u.typeany;
    const struct instruction_type012 * const t = &i->insn.u.type012;

    const int32_t  imm   = g->p == 3 ? i->insn.u.type3.imm : i->insn.u.type012.imm;
    const unsigned width = g->p == 3 ? MEDIUM_IMMEDIATE_BITWIDTH : SMALL_IMMEDIATE_BITWIDTH;
    const enum op  op    = g->p == 3 ? OP_BITWISE_OR : t->op;

    const int fields[3]   = { imm, t->y, t->x };
    const int * const map = asm_op_loc[g->p];
    const int lzero       = fields[map[2]] == 0;
    const int mzero       = fields[map[1]] == 0;
    const int rzero       = fields[map[0]] == 0;

    const struct hides hid = hide[g->p][op_types[op]][lzero][mzero][rzero];

    const int verbose = !!(flags & ASM_VERBOSE);
    const int decimal = !!(flags & ASM_FORCE_DECIMAL_CONSTANTS);

    const char           c0    = brackets[0][g->dd == 2]; // left side deref ?
    const char           c1    = 'A' + g->z;              // register name for Z
    const char           c2    = brackets[1][g->dd == 2]; // left side deref ?
    const char   * const s3    = arrows[g->dd];           // arrow direction
    const char           c4    = brackets[0][g->dd & 1];  // right side deref ?
    const char           s5[2] = { 'A' + g->x };          // register name for X
    const char   *       s6    = op_names[op];            // operator name
    const char           s7[2] = { 'A' + t->y };          // register name for Y
    const int32_t        i8    = SEXTEND32(width,imm);    // immediate value
    const char           c9    = brackets[1][g->dd & 1];  // right side deref ?

    const int show1 = (!hid.a || verbose) && g->p != 3;
    const int show2 = (!hid.b || verbose);
          int show3 = (!hid.c || verbose);
    const int flip  = g->p == 3 && i8 < 0 && show2 && !verbose;

    char s8[16];
    const char * const numfmt = (!decimal && verbose) ? "0x%08x" : "%d";
    snprintf(s8, sizeof s8, numfmt, flip ? -i8 : i8);

    const char * const ss[] = { s8, s7, s5 };
    const char *       sA   = ss[map[2]]; // can be overwritten by syntax sugar
    const char * const sB   = ss[map[1]];
    const char * const sC   = ss[map[0]];
    const char         sF[] = { ' ', "+-"[flip], ' ', '\0' }; // type0 sugar

    // Syntax sugars. type1 is not eligible (~0 and -0 become literals)
    if (lzero && !verbose && g->p != 1) {
        switch (op) {
            case OP_BITWISE_ORN:    // `B <- ~ C`
                s6 = "~";
                // FALLTHROUGH
            case OP_SUBTRACT:       // `B <- - C`
                sA = " ";
                show3 |= g->p == 0; // append term to disambiguate type0, type2
                break;

            case OP_BITWISE_OR:
            case OP_BITWISE_AND:
            case OP_BITWISE_XOR:
            case OP_SHIFT_RIGHT_ARITH:
            case OP_ADD:
            case OP_MULTIPLY:
            case OP_COMPARE_EQ:
            case OP_COMPARE_LT:
            //case OP_BITWISE_ORN:
            case OP_BITWISE_ANDN:
            case OP_PACK:
            case OP_SHIFT_RIGHT_LOGIC:
            //case OP_SUBTRACT:
            case OP_SHIFT_LEFT:
            case OP_TEST_BIT:
            case OP_COMPARE_GE:
                break;
        }
    }

    // Centre a 1-to-3-character op
    char opstr[MAX_OP_LEN + 1];
    snprintf(opstr, sizeof opstr, "%-2s", s6);
    s6 = opstr;

    #define C_(C,B,A) (((C) << 2) | ((B) << 1) | (A))
    switch (C_(show1,show2,show3)) {
        #define PUT(...) \
            out->op.fprintf(out,disasm_fmts[show1 + show2 + show3],__VA_ARGS__)
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

// LCOV_EXCL_START
        default:
            fatal(0, "Unsupported show1,show2,show3 %d,%d,%d",
                    show1,show2,show3);
// LCOV_EXCL_STOP
    }
    #undef C_
}

int print_registers(STREAM *out, const int32_t regs[16])
{
    for (int i = 0; i < 15; i++) {
        out->op.fprintf(out, "%c %08x ", 'A' + i, regs[i]);
        if (i % 6 == 5)
            out->op.fwrite("\n", 1, 1, out);
    }

    // Treat P specially : a read would return IP + 1
    out->op.fprintf(out, "%c %08x ", 'A' + 15, regs[15] + 1);

    out->op.fwrite("\n", 1, 1, out);

    return 0;
}

/*******************************************************************************
 * general hooks
 */
static int gen_init(STREAM *stream, struct param_state *p, void **ud)
{
    int *offset = *ud = malloc(sizeof *offset);
    *offset = 0;
    return 0;
}

static int gen_fini(STREAM *stream, void **ud)
{
    free(*ud);
    *ud = NULL;
    return 0;
}

/*******************************************************************************
 * Object format : simple section-based objects
 */
struct obj_fdata {
    struct obj *o;
    long syms;
    long rlcs;

    struct objrec *curr_rec;

    struct objsym **next_sym;
    struct objrlc **next_rlc;
    SWord pos;   ///< position in objrec

    int assembling;
    int error;
};

static int obj_init(STREAM *stream, struct param_state *p, void **ud)
{
    int rc = 0;

    struct obj_fdata *u = *ud = calloc(1, sizeof *u);
    struct obj *o = u->o = calloc(1, sizeof *o);

    o->magic.parsed.version = 2;
    param_get_int(p, "assembling", &u->assembling);

    if (u->assembling) {
        // TODO proper multiple-records support
        o->rec_count = 1;
        o->records = calloc((size_t)o->rec_count, sizeof *o->records);
        o->records[0].addr = 0;
        o->records[0].size = 1024; // will be realloc()ed as appropriate
        o->records[0].data = calloc((size_t)o->records->size, sizeof *o->records->data);
        u->curr_rec = o->records;

        u->next_sym = &o->symbols;
        u->next_rlc = &o->relocs;
    } else {
        rc = obj_read(u->o, stream);
        u->curr_rec = o->records;
    }

    return rc;
}

static int obj_in(STREAM *stream, struct element *i, void *ud)
{
    struct obj_fdata *u = ud;

    struct objrec *rec = u->curr_rec;
    int done = 0;
    while (!done) {
        if (!rec) {
            // We have reached the end of the list. This is not an error
            // condition, but we need to signal it specially.
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
        rec->data = realloc(rec->data, (size_t)rec->size * sizeof *rec->data);
    }

    // We could store .zero data sparsely, but we don't (yet)
    memset(&rec->data[u->pos], 0x00, (size_t)i->insn.size);
    rec->data[u->pos] = i->insn.u.word;
    u->pos += i->insn.size;
}

static int obj_out(STREAM *stream, struct element *i, void *ud)
{
    struct obj_fdata *u = ud;

    obj_out_insn(i, u, (struct obj*)u->o);

    return 1;
}

static int obj_sym(STREAM *stream, struct symbol *symbol, int flags, void *ud)
{
    int rc = 1;
    struct obj_fdata *u = ud;

    if (symbol->global) {
        struct objsym *sym = *u->next_sym = calloc(1, sizeof *sym);

        sym->name.str = strdup_rounded_up(symbol->name);
        sym->name.len = (SWord)strlen(symbol->name);
        // `symbol->resolved` must be true by this point
        sym->value = symbol->reladdr;
        sym->flags = flags;

        u->next_sym = &sym->next;
        u->syms++;
    }

    return rc;
}

static int obj_reloc(STREAM *stream, struct reloc_node *reloc, void *ud)
{
    struct obj_fdata *u = ud;
    if (!reloc || !reloc->insn) {
        u->error = 1;
        return 0;
    }

    struct objrlc *rlc = *u->next_rlc = calloc(1, sizeof *rlc);

    rlc->flags = (SWord)reloc->flags;
    rlc->name.str  = reloc->name ? strdup_rounded_up(reloc->name) : NULL;
    rlc->name.len  = reloc->name ? (SWord)strlen(reloc->name) : 0;
    rlc->addr = reloc->insn->insn.reladdr;
    rlc->width = reloc->width;
    rlc->shift = reloc->shift;

    u->next_rlc = &rlc->next;

    u->rlcs++;

    return 1;
}

static int obj_emit(STREAM *stream, void **ud)
{
    struct obj_fdata *u = *ud;
    struct obj *o = u->o;

    if (u->assembling) {
        o->records[0].size = u->pos;
        o->sym_count = (SWord)u->syms;
        o->rlc_count = (SWord)u->rlcs;

        obj_write(u->o, stream);
    }

    return 0;
}

static int obj_fini(STREAM *stream, void **ud)
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
 * Text format : hexadecimal numbers
 */
static int text_in(STREAM *stream, struct element *i, void *ud)
{
    int32_t *offset = ud;
    int result = stream->op.fscanf(stream, "  %x", &i->insn.u.word) == 1;
    if (!result) {
        if (stream->op.feof(stream))
            return -1;
        result = stream->op.fscanf(stream, " 0x%x", &i->insn.u.word) == 1;
        if (!result && stream->op.feof(stream))
            return -1;
    }
    i->insn.reladdr = (*offset)++;
    // Check for whitespace or EOF after the consumed item. This format can
    // read "agda"" as "0xa" and subsequently fail, when the whole string
    // should have been rejected.
    char next_char = 0;
    size_t len = stream->op.fread(&next_char, 1, 1, stream);
    if (len > 0 && !isspace(next_char))
        return -1;
    return result ? 1 : -1;
}

static int text_out(STREAM *stream, struct element *i, void *ud)
{
    int ok = 1;
    if (i->insn.size > 0)
        ok &= stream->op.fprintf(stream, "0x%08x\n", i->insn.u.word) > 0;
    for (int c = 1; c < i->insn.size && ok; c++)
        ok &= stream->op.fprintf(stream, "0x00000000\n") > 0;
    return ok ? 1 : -1;
}

/*******************************************************************************
 * memh format : suitable for use with $readmemh() in Verilog
 */

struct memh_state {
    int32_t written, marked, offset;
    int emit_zeros;
    bool first_done;
    bool error;
};

static int memh_init(STREAM *stream, struct param_state *p, void **ud)
{
    struct memh_state *state = *ud = calloc(1, sizeof *state);

    state->offset = 0;
    param_get_int(p, "format.memh.offset", &state->offset);
    state->marked = state->written = state->offset;

    param_get_int(p, "format.memh.explicit", &state->emit_zeros);

    return 0;
}

static int memh_in(STREAM *stream, struct element *i, void *ud)
{
    struct memh_state *state = ud;

    if (state->marked > state->written) {
        i->insn.u.word = 0x0;
        i->insn.reladdr = state->written++;
        return 1;
    } else if (state->marked == state->written) {
        if (stream->op.fscanf(stream, " @%x", &state->marked) == 1)
            return 0; // let next call handle it

        if (stream->op.fscanf(stream, " %x", &i->insn.u.word) != 1)
            return -1;

        i->insn.reladdr = state->written++;
        state->marked = state->written;
        return 1;
    } else {
        state->error = 1;
        return -1; // @address moved backward (unsupported)
    }
}

static int memh_out(STREAM *stream, struct element *i, void *ud)
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
        printed = stream->op.fprintf(stream, "@%x ", addr) > 2;

    state->first_done = 1;

    return (printed + stream->op.fprintf(stream, "%08x\n", word) > 3) ? 1 : -1;
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
    { "text",
        .init  = gen_init,
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

size_t make_format_list(int (*pred)(const struct format *), size_t flen,
        const struct format *fmts, size_t len, char *buf, const char *sep)
{
    size_t pos = 0;
    for (const struct format *f = fmts; pos < len && f < fmts + flen; f++)
        if (!pred || pred(f))
            pos += (size_t)snprintf(&buf[pos], len - pos, "%s%s", pos ? sep : "", f->name);

    return pos;
}

static int find_format_by_name(const void *_a, const void *_b)
{
    const struct format *a = _a, *b = _b;
    return strcmp(a->name, b->name);
}

int find_format(const char *optarg, const struct format **f)
{
    lfind_size_t sz = tenyr_asm_formats_count;
    *f = lfind(&(struct format){ .name = optarg }, tenyr_asm_formats, &sz,
            sizeof tenyr_asm_formats[0], find_format_by_name);
    return !*f;
}

/* vi: set ts=4 sw=4 et: */
