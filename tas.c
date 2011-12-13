#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>

#include "ops.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"
#include "common.h"

static const char *op_names[] = {
    [OP_BITWISE_OR         ] = "|",
    [OP_BITWISE_AND        ] = "&",
    [OP_ADD                ] = "+",
    [OP_MULTIPLY           ] = "*",
    [OP_MODULUS            ] = "%",
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
};

int print_disassembly(FILE *out, struct instruction *i)
{
    switch (i->u._xxxx.t) {
        case 0b1010:
        case 0b1000: {
            struct instruction_load_immediate_unsigned *g = &i->u._10x0;
            fprintf(out, "%c%c%c <-  0x%08x\n",
                    g->d ? '[' : ' ',   // left side dereferenced ?
                    'A' + g->z,         // register name for Z
                    g->d ? ']' : ' ',   // left side dereferenced ?
                    g->imm              // immediate value
                );
            return 0;
        }
        case 0b1011:
        case 0b1001: {
            struct instruction_load_immediate_signed *g = &i->u._10x1;
            fprintf(out, "%c%c%c <-  $ 0x%08x\n",
                    g->d ? '[' : ' ',   // left side dereferenced ?
                    'A' + g->z,         // register name for Z
                    g->d ? ']' : ' ',   // left side dereferenced ?
                    g->imm              // immediate value
                );
            return 0;
        }
        case 0b0000 ... 0b0111: {
            struct instruction_general *g = &i->u._0xxx;
            int ld = g->dd & 2;
            int rd = g->dd & 1;
            fprintf(out, "%c%c%c %s %c%c %-2s %c + 0x%08x%c\n",
                    ld ? '[' : ' ',     // left side dereferenced ?
                    'A' + g->z,         // register name for Z
                    ld ? ']' : ' ',     // left side dereferenced ?
                    g->r ? "->" : "<-", // arrow direction
                    rd ? '[' : ' ',     // right side dereferenced ?
                    'A' + g->x,         // register name for X
                    op_names[g->op],    // operator name
                    'A' + g->y,         // register name for Y
                    g->imm,             // immediate value
                    rd ? ']' : ' '      // right side dereferenced ?
                );

            return 0;
        }
    }

    return -1;
}

static void binary(FILE *stream, struct instruction *i)
{
    fwrite(&i->u.word, sizeof i->u.word, 1, stream);
}

static void text(FILE *stream, struct instruction *i)
{
    fprintf(stream, "0x%08x\n", i->u.word);
}

static const char shortopts[] = "df:o:hV";

static const struct option longopts[] = {
    { "disassemble",       no_argument, NULL, 'd' },
    { "format"     , required_argument, NULL, 'f' },
    { "output"     , required_argument, NULL, 'o' },

    { "help"       ,       no_argument, NULL, 'h' },
    { "version"    ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

static const char *version()
{
    return "tas version " STR(BUILD_NAME);
}

static int usage(const char *me)
{
    printf("Usage:\n"
           "  %s [ OPTIONS ] assembly-or-image-file [ assembly-or-image-file ... ] \n"
           "  -d, --disassemble     disassemble (default is to assemble)\n"
           "  -f, --format=F        select output format ('binary' or 'text')\n"
           "  -o, --output=X        write output to filename X\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string '%s'\n"
           , me, version());

    return 0;
}

struct format {
    const char *name;
    void (*impl)(FILE *, struct instruction *);
};

static int find_format_by_name(const void *_a, const void *_b)
{
    const struct format *a = _a, *b = _b;
    return strcmp(a->name, b->name);
}

static int eval_ce(struct const_expr *ce, uint32_t *result)
{
    uint32_t left, right;

    switch (ce->type) {
        case LAB: *result = 0          ; return 0; // TODO look up label
        case ICI: *result = ce->reladdr; return 0;
        case IMM: *result = ce->i.i    ; return 0;
        case OP2:
            if (!eval_ce(ce->left, &left) && !eval_ce(ce->right, &right)) {
                switch (ce->op) {
                    case '+': *result = left + right; return 0;
                    case '-': *result = left - right; return 0;
                    case '*': *result = left * right; return 0;
                    default: abort(); // TODO handle more gracefully
                }
            }
            return 1;
        default:
            abort(); // TODO handle more gracefully
    }
}

static int fixup_relocations(struct relocation_list *r)
{
    while (r) {
        struct const_expr *ce = r->ce;

        uint32_t result;
        if (!eval_ce(ce, &result))
            fprintf(stderr, "%d\n", result);

        r = r->next;
    }

    return 0;
}

int do_assembly(FILE *in, FILE *out, const struct format *f)
{
    struct parse_data pd = {
        .top = NULL,
    };

    tenor_lex_init(&pd.scanner);
    tenor_set_extra(&pd, pd.scanner);

    if (in)
        tenor_set_in(in, pd.scanner);
        //tenor_restart(in, pd.scanner); // TODO ?

    int result = tenor_parse(&pd);
    if (!result && f) {
        struct instruction_list *p = pd.top, *q = p;
        fixup_relocations(pd.relocs);

        while (q) {
            struct instruction_list *t = q;
            f->impl(out, q->insn);
            q = q->next;
            free(t);
        }
    }
    tenor_lex_destroy(pd.scanner);

    return 0;
}

int do_disassembly(FILE *in, FILE *out)
{
    struct instruction i;
    while (fread(&i.u.word, sizeof i.u.word, 1, in) == 1) {
        print_disassembly(out, &i);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    int disassemble = 0;

    struct format formats[] = {
        { "binary", binary },
        { "text"  , text   },
    };

    FILE *out = stdout;
    const struct format *f = &formats[0];

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'o': out = fopen(optarg, "w"); break;
            case 'd': disassemble = 1; break;
            case 'f': {
                size_t sz = countof(formats);
                f = lfind(&(struct format){ .name = optarg }, formats, &sz,
                        sizeof formats[0], find_format_by_name);
                if (!f)
                    exit(usage(argv[0]));

                break;
            }
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
    }

    for (int i = optind; i < argc; i++) {
        FILE *in = stdin;
        if (!out) {
            perror("Failed to open output file");
            return EXIT_FAILURE;
        }

        if (!strcmp(argv[i], "-")) {
            in = stdin;
        } else {
            in = fopen(argv[i], "r");
            if (!in) {
                char buf[128];
                snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[i]);
                perror(buf);
                return EXIT_FAILURE;
            }
        }

        if (disassemble)
            do_disassembly(in, out);
        else
            do_assembly(in, out, f);
    }

    fclose(out);

    return rc;
}

