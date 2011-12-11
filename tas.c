#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>

#include "ops.h"
#include "parser.h"
#include "parser_global.h"

#define countof(X) (sizeof (X) / sizeof (X)[0])

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
            fprintf(out, "%c%c%c %s %c%c %-3s %c + 0x%08x%c\n",
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
    fwrite(i, 4, 1, stream);
}

static void text(FILE *stream, struct instruction *i)
{
    fprintf(stream, "0x%08x\n", i->u.word);
}

static const char shortopts[] = "df:o:h"; //V

static const struct option longopts[] = {
    { "disassemble",       no_argument, NULL, 'd' },
    { "format"     , required_argument, NULL, 'f' },
    { "output"     , required_argument, NULL, 'o' },

    { "help"       ,       no_argument, NULL, 'h' },
    //{ "version"    ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 }
};

static int usage(const char *me)
{
    printf("Usage:\n"
           "  %s [ OPTIONS ]\n"
           , me);

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

int do_assembly(FILE *out, const struct format *f)
{
    int yyparse(void);
    int result = yyparse();
    if (!result && f) {
        struct instruction_list *p = tenor_get_parser_result(), *q = p;

        while (q) {
            struct instruction_list *t = q;
            f->impl(out, q->insn);
            q = q->next;
            free(t);
        }
    }

    return 0;
}

int do_disassembly(FILE *out)
{
    char buf[64];
    while (fread(buf, 4, 1, stdin) == 1) {
        print_disassembly(out, (void*)buf);
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
            }
            case 'h':
                usage(argv[0]);
                return EXIT_FAILURE;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (!out) {
        perror("Failed to open output file");
        return EXIT_FAILURE;
    }

    if (disassemble)
        do_disassembly(out);
    else
        do_assembly(out, f);

    return rc;
}

