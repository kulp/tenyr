#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <search.h>

#include "ops.h"
#include "common.h"

#define PTR_MASK ~(-1 << 24)

struct state {
    uint32_t regs[16];
    uint32_t mem[1 << 24];
};

int run_instruction(struct state *s, struct instruction *i)
{
    int sextend = 0;
    uint32_t _scratch,
             *ip = &s->regs[15];

    assert(("PC within address space", !(s->regs[15] & ~PTR_MASK)));

    switch (i->u._xxxx.t) {
        case 0b1011:
        case 0b1001:
            sextend = 1;
        case 0b1010:
        case 0b1000: {
            struct instruction_load_immediate_signed *g = &i->u._10x1;
            uint32_t *Z = &s->regs[g->z];
            if (g->d) Z = &s->mem[*Z & PTR_MASK];
            uint32_t imm = sextend ? g->imm : (int32_t)(uint32_t)g->imm;
            *Z = imm;
            break;
        }
        case 0b0000 ... 0b0111: {
            struct instruction_general *g = &i->u._0xxx;
            uint32_t *Z = &s->regs[g->z];
            uint32_t  X =  s->regs[g->x];
            uint32_t  Y =  s->regs[g->y];
            int32_t   I = g->imm;
            uint32_t _rhs, *rhs = &_rhs;

            switch (g->op) {
                case OP_BITWISE_OR          : *rhs =  (X  |  Y) + I; break;
                case OP_BITWISE_AND         : *rhs =  (X  &  Y) + I; break;
                case OP_ADD                 : *rhs =  (X  +  Y) + I; break;
                case OP_MULTIPLY            : *rhs =  (X  *  Y) + I; break;
                case OP_MODULUS             : *rhs =  (X  %  Y) + I; break;
                case OP_SHIFT_LEFT          : *rhs =  (X  << Y) + I; break;
                case OP_COMPARE_LTE         : *rhs = -(X  <= Y) + I; break;
                case OP_COMPARE_EQ          : *rhs = -(X  == Y) + I; break;
                case OP_BITWISE_NOR         : *rhs = ~(X  |  Y) + I; break;
                case OP_BITWISE_NAND        : *rhs = ~(X  &  Y) + I; break;
                case OP_BITWISE_XOR         : *rhs =  (X  ^  Y) + I; break;
                case OP_ADD_NEGATIVE_Y      : *rhs =  (X  + -Y) + I; break;
                case OP_XOR_INVERT_X        : *rhs =  (X  ^ ~Y) + I; break;
                case OP_SHIFT_RIGHT_LOGICAL : *rhs =  (X  >> Y) + I; break;
                case OP_COMPARE_GT          : *rhs = -(X  >  Y) + I; break;
                case OP_COMPARE_NE          : *rhs = -(X  != Y) + I; break;
            }

            if (g->dd & 2) Z   = &s->mem[*Z   & PTR_MASK];
            if (g->dd & 1) rhs = &s->mem[*rhs & PTR_MASK];

            {
                uint32_t *r, *w;
                if (g->r)
                    r = Z, w = rhs;
                else
                    w = Z, r = rhs;

                if (w == ip) ip = &_scratch; // throw away later update
                if (w == &s->regs[0]) w = &_scratch;  // throw away write to reg 0
                *w = *r;
            }

            break;
        }
        default: abort();
    }

    *ip += 1;
    // TODO trap wrap
    *ip &= PTR_MASK;

    return 0;
}

static int binary(FILE *in, struct instruction **insn)
{
    struct instruction *i = *insn = malloc(sizeof *i);
    return fread(&i->u.word, 4, 1, in) == 1;
}

static int text(FILE *in, struct instruction **insn)
{
    struct instruction *i = *insn = malloc(sizeof *i);
    return fscanf(in, "%x", &i->u.word) == 1;
}

static const char shortopts[] = "a:s:f:vhV";

static const struct option longopts[] = {
    { "address"    , required_argument, NULL, 'a' },
    { "format"     , required_argument, NULL, 'f' },
    { "verbose"    ,       no_argument, NULL, 'v' },

    { "help"       ,       no_argument, NULL, 'h' },
    { "version"    ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

static const char *version()
{
    return "tsim version " STR(BUILD_NAME);
}

static int usage(const char *me)
{
    printf("Usage:\n"
           "  %s [ OPTIONS ] imagefile\n"
           "  -a, --address=N       load instructions into memory at word address N\n"
           "  -s, --start=N         start execution at word address N\n"
           "  -f, --format=F        select input format ('binary' or 'text')\n"
           "  -v, --verbose         increase verbosity of output\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string '%s'\n"
           , me, version());

    return 0;
}

struct format {
    const char *name;
    int (*impl)(FILE *, struct instruction **);
};

static int find_format_by_name(const void *_a, const void *_b)
{
    const struct format *a = _a, *b = _b;
    return strcmp(a->name, b->name);
}

int main(int argc, char *argv[])
{
    struct state *s = calloc(1, sizeof *s);
    int load_address = 0, start_address = 0;
    int verbose = 0;

    struct format formats[] = {
        { "binary", binary },
        { "text"  , text   },
    };

    const struct format *f = &formats[0];

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'a': load_address = strtol(optarg, NULL, 0); break;
            case 's': start_address = strtol(optarg, NULL, 0); break;
            case 'f': {
                size_t sz = countof(formats);
                f = lfind(&(struct format){ .name = optarg }, formats, &sz,
                        sizeof formats[0], find_format_by_name);
                if (!f)
                    exit(usage(argv[0]));

                break;
            }
            case 'v': verbose++; break;

            case 'V': puts(version()); return EXIT_SUCCESS;
            case 'h':
                usage(argv[0]);
                return EXIT_FAILURE;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "No input files specified on the command line\n");
        exit(usage(argv[0]));
    } else if (argc - optind > 1) {
        fprintf(stderr, "More than one input file specified on the command line\n");
        exit(usage(argv[0]));
    }

    FILE *in = stdin;

    if (!strcmp(argv[optind], "-")) {
        in = stdin;
    } else {
        in = fopen(argv[optind], "r");
        if (!in) {
            char buf[128];
            snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[optind]);
            perror(buf);
            return EXIT_FAILURE;
        }
    }

    struct instruction *i;
    while (f->impl(in, &i) > 0) {
        s->mem[load_address++] = i->u.word;
        free(i);
    }

    int print_disassembly(FILE *out, struct instruction *i);
    int print_registers(FILE *out, uint32_t regs[16]);
    s->regs[15] = start_address & PTR_MASK;
    while (1) {
        if (verbose > 0)
            printf("IP = 0x%06x\t", s->regs[15]);

        assert(("PC within address space", !(s->regs[15] & ~PTR_MASK)));
        // TODO make it possible to cast memory location to instruction again
        struct instruction i = { .u.word = s->mem[ s->regs[15] ] };

        if (verbose > 1)
            print_disassembly(stdout, &i);
        if (verbose > 2)
            fputs("\n", stdout);
        if (verbose > 2)
            print_registers(stdout, s->regs);
        if (verbose > 0)
            fputs("\n", stdout);

        run_instruction(s, &i);
    }

    return 0;
}

