/* required to use popen() and friends with -std=c99 set */
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>


#include "ops.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"
#include "common.h"
#include "asm.h"

int print_disassembly(FILE *out, struct instruction *i);

static const char shortopts[] = "df:o:p::hV";

static const struct option longopts[] = {
    { "disassemble" ,       no_argument, NULL, 'd' },
    { "format"      , required_argument, NULL, 'f' },
    { "output"      , required_argument, NULL, 'o' },
    { "preprocessor", optional_argument, NULL, 'p' },

    { "help"        ,       no_argument, NULL, 'h' },
    { "version"     ,       no_argument, NULL, 'V' },

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
           "  -p, --preprocessor=X  pass input through preprocessor first (defaults to `cpp')\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string '%s'\n"
           , me, version());

    return 0;
}

static int label_lookup(struct label_list *node, const char *name, uint32_t *result)
{
    while (node) {
        // TODO strcasecmp ?
        if (!strcasecmp(node->label->name, name)) {
            *result = node->label->reladdr;
            return 0;
        }

        node = node->next;
    }

    return 1;
}

static int ce_eval(struct parse_data *pd, struct instruction *top_insn, struct
        const_expr *ce, uint32_t *result)
{
    uint32_t left, right;

    switch (ce->type) {
        case LAB: return label_lookup(pd->labels, ce->labelname, result);
        case ICI: *result = top_insn->reladdr; return 0;
        case IMM: *result = ce->i; return 0;
        case OP2:
            if (!ce_eval(pd, top_insn, ce->left, &left) &&
                !ce_eval(pd, top_insn, ce->right, &right))
            {
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

void ce_free(struct const_expr *ce, int recurse)
{
    if (recurse)
        switch (ce->type) {
            case LAB:
            case ICI:
                break;
            case IMM:
                free(ce->left);
                break;
            case OP2:
                ce_free(ce->left, recurse);
                ce_free(ce->right, recurse);
                break;
            default:
                abort(); // TODO handle more gracefully
        }

    free(ce);
}

static int fixup_relocations(struct parse_data *pd)
{
    int rc = 0;
    struct relocation_list *r = pd->relocs;

    while (r) {
        struct const_expr *ce = r->ce;

        uint32_t result;
        if ((rc = ce_eval(pd, ce->insn, ce, &result)) == 0) {
            // TODO check for resolvedness first
            uint32_t mask = -1ULL << r->width; // technically UB
            result *= r->mult;
            *r->dest &= mask;
            *r->dest |= result & ~mask;
            ce_free(ce, 1);
        } else {
            fprintf(stderr, "Error while fixing up relocations\n");
            // TODO print out information about the relocation
            return -1;
        }

        struct relocation_list *last = r;
        r = r->next;
        free(last);
    }

    return 0;
}

int do_assembly(FILE *in, FILE *out, const struct format *f)
{
    struct parse_data pd = { .top = NULL };

    tenyr_lex_init(&pd.scanner);
    tenyr_set_extra(&pd, pd.scanner);

    if (in)
        tenyr_set_in(in, pd.scanner);
        //tenyr_restart(in, pd.scanner); // TODO ?

    int result = tenyr_parse(&pd);
    if (!result && f) {
        struct instruction_list *p = pd.top, *q = p;

        int baseaddr = 0; // TODO
        int reladdr = 0;
        // first pass, fix up addresses
        while (q) {
            q->insn->reladdr = baseaddr + reladdr;
            struct label *l = q->insn->label;
            while (l) {
                if (!l->resolved) {
                    l->reladdr = baseaddr + reladdr;
                    l->resolved = 1;
                }
                l = l->next;
            }

            reladdr++;
            q = q->next;
        }

        if (!fixup_relocations(&pd)) {
            q = p;
            while (q) {
                struct instruction_list *t = q;
                f->impl_out(out, q->insn);
                q = q->next;
                free(t->insn);
                free(t);
            }
        }

        struct label_list *l = pd.labels, *last = l;
        while (l) {
            l = l->next;
            free(last->label);
            free(last);
            last = l;
        }
    }
    tenyr_lex_destroy(pd.scanner);

    return 0;
}

int do_disassembly(FILE *in, FILE *out, const struct format *f)
{
    struct instruction i;
    while (f->impl_in(in, &i) == 1) {
        print_disassembly(out, &i);
        fputs("\n", out);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    int disassemble = 0;
    const char *preprocessor = NULL;

    FILE *out = stdout;
    const struct format *f = &formats[0];

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'p': preprocessor = optarg ? optarg : "cpp"; break;
            case 'o': out = fopen(optarg, "w"); break;
            case 'd': disassemble = 1; break;
            case 'f': {
                size_t sz = formats_count;
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
        FILE *in = NULL;
        if (!out) {
            perror("Failed to open output file");
            return EXIT_FAILURE;
        }

        if (preprocessor) {
            char cmd[256];
            // TODO doesn't handle escaping of argument properly
            snprintf(cmd, sizeof cmd, "%s '%s'", preprocessor, argv[i]);
            in = popen(cmd, "r");
        } else  {
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
        }

        if (disassemble)
            do_disassembly(in, out, f);
        else
            do_assembly(in, out, f);

        if (preprocessor)
            pclose(in);
        else
            fclose(in);
    }

    fclose(out);

    return rc;
}

