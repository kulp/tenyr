#include "ops.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"
#include "common.h"
#include "asm.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>

int print_disassembly(FILE *out, struct instruction *i);

static const char shortopts[] = "df:o::hV";

static const struct option longopts[] = {
    { "disassemble" ,       no_argument, NULL, 'd' },
    { "format"      , required_argument, NULL, 'f' },
    { "output"      , required_argument, NULL, 'o' },

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
           "  -f, --format=F        select output format (binary, text, obj)\n"
           "  -o, --output=X        write output to filename X\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string '%s'\n"
           , me, version());

    return 0;
}

static int label_find(struct label_list *list, const char *name, struct label **label)
{
    while (list) {
        if (!strncmp(list->label->name, name, LABEL_LEN)) {
            *label = list->label;
            return 0;
        }

        list = list->next;
    }

    return 1;
}

static int label_lookup(struct label_list *list, const char *name, uint32_t *result)
{
    struct label *label = NULL;
    if (!label_find(list, name, &label)) {
        *result = label->reladdr;
        return 0;
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
                    case '+': *result = left +  right; return 0;
                    case '-': *result = left -  right; return 0;
                    case '*': *result = left *  right; return 0;
                    case LSH: *result = left << right; return 0;
                    default: fatal("Unrecognised const_expr op", 0);
                }
            }
            return 1;
        default:
            fatal("Unrecognised const_expr type", 0);
            return 1;
    }
}

static void ce_free(struct const_expr *ce, int recurse)
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
                fatal("Unrecognised const_expr type", 0);
        }

    free(ce);
}

static int fixup_deferred_exprs(struct parse_data *pd)
{
    int rc = 0;
    struct deferred_expr *r = pd->defexprs;

    while (r) {
        struct const_expr *ce = r->ce;

        uint32_t result;
        if ((rc = ce_eval(pd, ce->insn, ce, &result)) == 0) {
            // TODO check for resolvedness first
            uint32_t mask = ~((1ULL << r->width) - 1);
            result *= r->mult;
            *r->dest &= mask;
            *r->dest |= result & ~mask;
            ce_free(ce, 1);
        } else {
            fatal("Error while fixing up deferred expressions", 0);
            // TODO print out information about the deferred expression
        }

        struct deferred_expr *last = r;
        r = r->next;
        free(last);
    }

    return 0;
}

static int mark_globals(struct label_list *labels, struct global_list *globals)
{
    struct label *which;
    while (globals) {
        if (!label_find(labels, globals->name, &which))
            which->global = 1;
        globals = globals->next;
    }

    return 0;
}

static int check_labels(struct label_list *labels)
{
    int rc = 0;
    struct label_list *top = labels;
    typedef int cmp(const void *, const void*);

    // check for and reject duplicates
    void *tree = NULL;
    list_foreach(label_list, Node, labels) {
        const char **name = tsearch(Node->label->name, &tree, (cmp*)strcmp);

        if (*name != Node->label->name) {
            rc = 1;
            goto cleanup; // take that, district !
        }
    }

cleanup:
    // delete from tree what we added to it
    list_foreach(label_list, Node, top) {
        if (!tree) break;
        tdelete(Node->label, &tree, (cmp*)strcmp);
        Node = Node->next;
    }

    return rc;
}

int do_assembly(FILE *in, FILE *out, const struct format *f)
{
    struct parse_data pd = { .top = NULL };

    tenyr_lex_init(&pd.scanner);
    tenyr_set_extra(&pd, pd.scanner);

    if (in)
        tenyr_set_in(in, pd.scanner);

    int result = tenyr_parse(&pd);
    if (!result && f) {
        int baseaddr = 0; // TODO
        int reladdr = 0;
        // first pass, fix up addresses
        list_foreach(instruction_list, il, pd.top) {
            il->insn->reladdr = baseaddr + reladdr;

            list_foreach(label, l, il->insn->label) {
                if (!l->resolved) {
                    l->reladdr = baseaddr + reladdr;
                    l->resolved = 1;
                }
            }

            reladdr++;
        }

        mark_globals(pd.labels, pd.globals);
        // TODO make check_labels() more user-friendly
        if (check_labels(pd.labels))
            fatal("Error while processing labels : check for duplicate labels", 0);

        if (!fixup_deferred_exprs(&pd)) {
            void *ud;
            if (f->init)
                f->init(out, ASM_ASSEMBLE, &ud);

            list_foreach(instruction_list, Node, pd.top) {
                f->out(out, Node->insn, ud),
                free(Node->insn),
                free(Node);
            }

            if (f->fini)
                f->fini(out, &ud);
        }

        list_foreach(label_list, Node, pd.labels) {
            free(Node->label);
            free(Node);
        }

        list_foreach(global_list, Node, pd.globals)
            free(Node);
    }
    tenyr_lex_destroy(pd.scanner);

    return 0;
}

int do_disassembly(FILE *in, FILE *out, const struct format *f)
{
    struct instruction i;
    void *ud;
    if (f->init)
        f->init(in, ASM_DISASSEMBLE, &ud);

    while (f->in(in, &i, ud) == 1) {
        print_disassembly(out, &i);
        fputs("\n", out);
    }

    if (f->fini)
        f->fini(in, &ud);

    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    int disassemble = 0;

    const struct format *f = &formats[0];

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0]);
        return EXIT_FAILURE;
    }

    FILE *out = stdout;

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
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
        fatal("No input files specified on the command line", DISPLAY_USAGE);
    }

    for (int i = optind; i < argc; i++) {
        FILE *in = NULL;
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
            do_disassembly(in, out, f);
        else
            do_assembly(in, out, f);

        fclose(in);
    }

    fclose(out);

    return rc;
}

