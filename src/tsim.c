#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <search.h>

#include "ops.h"
#include "common.h"
#include "asm.h"

typedef int map_init(void *cookie, ...);
typedef int map_op(void *cookie, int op, uint32_t addr, uint32_t *data);

extern map_op ram_init;
extern map_init ram_op;

struct device {
    map_init *init;
    map_op *op;
    void *cookie;
};

struct state {
    struct {
        int abort;
    } conf;

    struct device devices[1]; // XXX

    int32_t regs[16];
    //int32_t mem[1 << 24];
};

static inline int dispatch_op(struct state *s, int op, uint32_t addr, uint32_t *data)
{
    // TODO choose devices
    struct device *device = &s->devices[0]; // XXX
    return device->op(device->cookie, op, addr, data);
}

int run_instruction(struct state *s, struct instruction *i)
{
    int32_t *ip = &s->regs[15];

    assert(("PC within address space", !(*ip & ~PTR_MASK)));

    int32_t *Z;
    int32_t _rhs, *rhs = &_rhs;
    uint32_t value;
    int deref_lhs, deref_rhs;
    int reversed;

    switch (i->u._xxxx.t) {
        case 0b1011:
        case 0b1001:
        case 0b1010:
        case 0b1000: {
            struct instruction_load_immediate *g = &i->u._10xx;
            Z = &s->regs[g->z];
            int32_t _imm = g->imm;
            //int32_t *imm = &_imm;
            deref_rhs = g->dd & 1;
            deref_lhs = g->dd & 2;
            reversed = 0;
            rhs = &_imm;

            break;
        }
        case 0b0000 ... 0b0111: {
            struct instruction_general *g = &i->u._0xxx;
            Z = &s->regs[g->z];
            int32_t  X =  s->regs[g->x];
            int32_t  Y =  s->regs[g->y];
            int32_t  I = SEXTEND(12, g->imm);
            deref_rhs = g->dd & 1;
            deref_lhs = g->dd & 2;
            reversed = !!g->r;

            switch (g->op) {
                case OP_BITWISE_OR          : *rhs =  (X  |  Y) + I; break;
                case OP_BITWISE_AND         : *rhs =  (X  &  Y) + I; break;
                case OP_ADD                 : *rhs =  (X  +  Y) + I; break;
                case OP_MULTIPLY            : *rhs =  (X  *  Y) + I; break;
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
                case OP_RESERVED            : goto bad;
            }

            break;
        }
        default:
            goto bad;
    }

    // common activity block
    {
        uint32_t r_addr = (reversed ? *Z   : *rhs) & PTR_MASK;
        uint32_t w_addr = (reversed ? *rhs : *Z  ) & PTR_MASK;

        int read_mem  = reversed ? deref_lhs : deref_rhs;
        int write_mem = reversed ? deref_rhs : deref_lhs;

        int32_t *r, *w;
        if (reversed)
            r = Z, w = rhs;
        else
            w = Z, r = rhs;

        if (read_mem)
            dispatch_op(s, 0, r_addr, &value);
        else
            value = *r;

        if (write_mem)
            dispatch_op(s, 1, w_addr, &value);
        else if (w != &s->regs[0])  // throw away write to reg 0
            *w = value;

        if (w != ip) {
            *ip += 1;
            if (*ip & ~PTR_MASK) goto bad; // trap wrap
            *ip &= PTR_MASK; // TODO right now this will never be reached
        }
    }

    return 0;

bad:
    if (s->conf.abort)
        abort();
    else
        return 1;
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

int main(int argc, char *argv[])
{
    int rc = EXIT_SUCCESS;

    struct state *s = calloc(1, sizeof *s);
    s->conf.abort = 0;
    int load_address = 0, start_address = 0;
    int verbose = 0;

    const struct format *f = &formats[0];

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'a': load_address = strtol(optarg, NULL, 0); break;
            case 's': start_address = strtol(optarg, NULL, 0); break;
            case 'f': {
                size_t sz = formats_count;
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
            rc = EXIT_FAILURE;
            goto done;
        }
    }

    // XXX
    struct device *device = &s->devices[0];
    device->op = ram_op;
    device->init = ram_init;

    device->init(&device->cookie);

    struct instruction i;
    while (f->impl_in(in, &i) > 0) {
        dispatch_op(s, 1, load_address++, &i.u.word);
        //s->mem[load_address++] = i.u.word;
    }

    s->regs[15] = start_address & PTR_MASK;
    while (1) {
        if (verbose > 0)
            printf("IP = 0x%06x\t", s->regs[15]);

        assert(("PC within address space", !(s->regs[15] & ~PTR_MASK)));
        // TODO make it possible to cast memory location to instruction again
        struct instruction i;
        dispatch_op(s, 0, s->regs[15], &i.u.word);

        if (verbose > 1)
            print_disassembly(stdout, &i);
        if (verbose > 2)
            fputs("\n", stdout);
        if (verbose > 2)
            print_registers(stdout, s->regs);
        if (verbose > 0)
            fputs("\n", stdout);

        if (run_instruction(s, &i))
            break;
    }

done:
    if (in)
        fclose(in);
    free(s);

    return rc;
}

