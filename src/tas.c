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

static const char shortopts[] = "df:o::hV";

static const struct option longopts[] = {
    { "disassemble" ,       no_argument, NULL, 'd' },
    { "format"      , required_argument, NULL, 'f' },
    { "output"      , required_argument, NULL, 'o' },

    { "help"        ,       no_argument, NULL, 'h' },
    { "version"     ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

#define version() "tas version " STR(BUILD_NAME)

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

struct symbol *symbol_find(struct symbol_list *list, const char *name)
{
    list_foreach(symbol_list, elt, list) {
        if (!strncmp(elt->symbol->name, name, SYMBOL_LEN)) {
            return elt->symbol;
        }
    }

    return NULL;
}

static int symbol_lookup(struct symbol_list *list, const char *name, uint32_t *result)
{
    struct symbol *symbol = NULL;
    if ((symbol = symbol_find(list, name))) {
        if (result) *result = symbol->reladdr;
        return 0;
    }

    // unresolved symbols get a zero value, but this is still success in EXT
    // case (not in SYM case)
    if (result) *result = 0;
    return 1;
}

static int add_relocation(struct parse_data *pd, const char *name, struct instruction *insn, int width)
{
    struct reloc_list *node = calloc(1, sizeof *node);

    if (name) {
        strncpy(node->reloc.name, name, sizeof node->reloc.name);
        node->reloc.name[sizeof node->reloc.name - 1] = 0;
    } else {
        node->reloc.name[0] = 0;
    }
    node->reloc.insn  = insn;
    node->reloc.width = width;

    node->next = pd->relocs;
    pd->relocs = node;

    insn->reloc = &node->reloc;

    return 0;
}

static int ce_eval(struct parse_data *pd, struct instruction *top_insn, struct
        const_expr *ce, int width, uint32_t *result)
{
    uint32_t left, right;

    switch (ce->type) {
        case EXT:
            if (symbol_lookup(pd->symbols, ce->symbolname, result))
                return add_relocation(pd, ce->symbolname, top_insn, width);
            else
                return add_relocation(pd, NULL, top_insn, width);
            return 0;
        case SYM: return symbol_lookup(pd->symbols, ce->symbolname, result);
        case ICI:
            *result = top_insn->reladdr;
            return add_relocation(pd, NULL, top_insn, width);
        case IMM: *result = ce->i; return 0;
        case OP2:
            if (!ce_eval(pd, top_insn, ce->left , width, &left) &&
                !ce_eval(pd, top_insn, ce->right, width, &right))
            {
                switch (ce->op) {
                    case '+': *result = left +  right; return 0;
                    case '-': *result = left -  right; return 0;
                    case '*': *result = left *  right; return 0;
                    case LSH: *result = left << right; return 0;
                    default: fatal(0, "Unrecognised const_expr op '%c'", ce->op);
                }
            }
            return 1;
        default:
            fatal(0, "Unrecognised const_expr type %d", ce->type);
            return 1;
    }
}

static void ce_free(struct const_expr *ce, int recurse)
{
    if (recurse)
        switch (ce->type) {
            case EXT:
            case SYM:
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
                fatal(0, "Unrecognised const_expr type %d", ce->type);
        }

    free(ce);
}

static int fixup_deferred_exprs(struct parse_data *pd)
{
    int rc = 0;

    list_foreach(deferred_expr, r, pd->defexprs) {
        struct const_expr *ce = r->ce;

        uint32_t result;
        if ((rc = ce_eval(pd, ce->insn, ce, r->width, &result)) == 0) {
            uint32_t mask = ~(((1ULL << (r->width - 1)) << 1) - 1);
            result *= r->mult;
            *r->dest &= mask;
            *r->dest |= result & ~mask;
            ce_free(ce, 1);
        } else {
            fatal(0, "Error while fixing up deferred expressions");
            // TODO print out information about the deferred expression
        }

        free(r);
    }

    return 0;
}

static int mark_globals(struct symbol_list *symbols, struct global_list *globals)
{
    struct symbol *which;
    list_foreach(global_list, g, globals)
        if ((which = symbol_find(symbols, g->name)))
            which->global = 1;

    return 0;
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

            list_foreach(symbol, l, il->insn->symbol) {
                if (!l->resolved) {
                    l->reladdr = baseaddr + reladdr;
                    l->resolved = 1;
                }
            }

            reladdr++;
        }

        mark_globals(pd.symbols, pd.globals);

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

        list_foreach(symbol_list, Node, pd.symbols) {
            free(Node->symbol);
            free(Node);
        }

        list_foreach(global_list, Node, pd.globals)
            free(Node);

        list_foreach(reloc_list, Node, pd.relocs)
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
        int len = print_disassembly(out, &i, ASM_AS_INSN);
        fprintf(out, "%*s# ", 30 - len, "");
        print_disassembly(out, &i, ASM_AS_DATA);
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
            case 'o': out = fopen(optarg, "wb"); break;
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

    if (optind >= argc)
        fatal(DISPLAY_USAGE, "No input files specified on the command line");

    for (int i = optind; i < argc; i++) {
        FILE *in = NULL;
        if (!out)
            fatal(PRINT_ERRNO, "Failed to open output file");

        if (!strcmp(argv[i], "-")) {
            in = stdin;
        } else {
            in = fopen(argv[i], "rb");
            if (!in) {
                char buf[128];
                snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[i]);
                fatal(PRINT_ERRNO, buf);
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

