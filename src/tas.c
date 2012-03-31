#include "ops.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"
#include "common.h"
#include "asm.h"

#include <assert.h>
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

static int ce_eval(struct parse_data *pd, struct instruction *context, struct
        const_expr *ce, int width, uint32_t *result);

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
    list_foreach(symbol_list, elt, list)
        if (!strncmp(elt->symbol->name, name, SYMBOL_LEN))
            return elt->symbol;

    return NULL;
}

static int symbol_lookup(struct parse_data *pd, struct symbol_list *list, const
        char *name, uint32_t *result)
{
    struct symbol *symbol = NULL;
    if ((symbol = symbol_find(list, name))) {
        if (result) {
            if (symbol->ce) {
                struct instruction_list *prev = *symbol->ce->deferred;
                struct instruction *c = prev ? prev->insn : NULL;
                return ce_eval(pd, c, symbol->ce, WORD_BITWIDTH, result);
            } else {
                *result = symbol->reladdr;
            }
        }
        return 0;
    }

    // unresolved symbols get a zero value, but this is still success in CE_EXT
    // case (not in CE_SYM case)
    if (result) *result = 0;
    return 1;
}

static int add_relocation(struct parse_data *pd, const char *name, struct instruction *insn, int width)
{
    struct reloc_list *node = calloc(1, sizeof *node);

    if (name && name[0]) {
        strcopy(node->reloc.name, name, sizeof node->reloc.name);
    } else {
        node->reloc.name[0] = 0;
    }
    node->reloc.insn  = insn;
    node->reloc.width = width;

    node->next = pd->relocs;
    pd->relocs = node;

    if (insn)
        insn->reloc = &node->reloc;

    return 0;
}

static int ce_eval(struct parse_data *pd, struct instruction *context, struct
        const_expr *ce, int width, uint32_t *result)
{
    uint32_t left, right;
    const char *name = ce->symbolname;
    if (ce->symbol)
        name = ce->symbol->name;

    switch (ce->type) {
        case CE_SYM:
        case CE_EXT:
            if (ce->symbol && ce->symbol->ce) {
                struct instruction_list *prev = *ce->symbol->ce->deferred;
                struct instruction *c = prev ? prev->insn : context;
                int rc = ce_eval(pd, c, ce->symbol->ce, width, result);
                if (c)
                    return add_relocation(pd, NULL, c, width);
                else
                    return rc;
            } else {
                int rc = symbol_lookup(pd, pd->symbols, name, result);
                if (ce->type == CE_SYM) {
                    return rc;
                } else
                    if (rc) {
                        return add_relocation(pd, name, context, width);
                    } else {
                        return add_relocation(pd, NULL, context, width);
                    }
            }
        case CE_ICI:
            if (context)
                *result = context->reladdr;
            else
                *result = 0;
            return add_relocation(pd, NULL, context, width);
        case CE_IMM: *result = ce->i; return 0;
        case CE_OP2:
            if (!ce_eval(pd, context, ce->left , width, &left) &&
                !ce_eval(pd, context, ce->right, width, &right))
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
    if (!ce)
        return;

    if (recurse)
        switch (ce->type) {
            case CE_EXT:
            case CE_SYM:
                if (ce->symbol && ce->symbol->ce) {
                    ce_free(ce->symbol->ce, recurse);
                    ce->symbol->ce = NULL;
                }
                break;
            case CE_ICI:
                break;
            case CE_IMM:
                free(ce->left);
                break;
            case CE_OP2:
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

static int check_symbols(struct symbol_list *symbols)
{
    int rc = 0;

    // check for and reject duplicates
    void *tree = NULL;
    list_foreach(symbol_list, Node, symbols) {
        if (!Node->symbol->unique)
            continue;

        const char **name = tsearch(Node->symbol->name, &tree, (cmp*)strcmp);

        if (*name != Node->symbol->name) {
            rc = 1;
            break;
        }
    }

    // delete from tree what we added to it
    list_foreach(symbol_list, Node, symbols) {
        if (!tree) break;
        tdelete(Node->symbol, &tree, (cmp*)strcmp);
        Node = Node->next;
    }

    return rc;
}

static int assembly_cleanup(struct parse_data *pd)
{
    list_foreach(instruction_list, Node, pd->top) {
        free(Node->insn);
        free(Node);
    }

    list_foreach(symbol_list, Node, pd->symbols) {
        ce_free(Node->symbol->ce, 1);
        free(Node->symbol);
        free(Node);
    }

    list_foreach(global_list, Node, pd->globals)
        free(Node);

    list_foreach(reloc_list, Node, pd->relocs)
        free(Node);

    return 0;
}

static int assembly_fixup_insns(struct parse_data *pd)
{
    int reladdr = 0;
    // first pass, fix up addresses
    list_foreach(instruction_list, il, pd->top) {
        if (!il->insn)
            continue;

        il->insn->reladdr = reladdr;

        list_foreach(symbol, l, il->insn->symbol) {
            if (!l->resolved) {
                l->reladdr = reladdr;
                l->resolved = 1;
            }
        }

        reladdr++;
    }

    list_foreach(symbol_list, li, pd->symbols)
        list_foreach(symbol, l, li->symbol)
            if (!l->resolved)
                if (!ce_eval(pd, NULL, l->ce, WORD_BITWIDTH, &l->reladdr))
                    l->resolved = 1;

    return 0;
}

static int assembly_inner(struct parse_data *pd, FILE *out, const struct format *f)
{
    assembly_fixup_insns(pd);

    mark_globals(pd->symbols, pd->globals);
    if (check_symbols(pd->symbols))
        fatal(0, "Error while processing symbols : check for duplicate symbols");

    if (!fixup_deferred_exprs(pd)) {
        void *ud;
        if (f->init)
            f->init(out, ASM_ASSEMBLE, &ud);

        list_foreach(instruction_list, Node, pd->top)
            // if !Node->insn, it's a placeholder or some kind of dummy
            if (Node->insn)
                f->out(out, Node->insn, ud);

        if (f->sym)
            list_foreach(symbol_list, Node, pd->symbols)
                f->sym(out, Node->symbol, ud);

        if (f->reloc)
            list_foreach(reloc_list, Node, pd->relocs)
                f->reloc(out, &Node->reloc, ud);

        if (f->fini)
            f->fini(out, &ud);
    }

    assembly_cleanup(pd);

    return 0;
}

int do_assembly(FILE *in, FILE *out, const struct format *f)
{
    struct parse_data _pd = { .top = NULL }, *pd = &_pd;

    tenyr_lex_init(&pd->scanner);
    tenyr_set_extra(pd, pd->scanner);

    if (in)
        tenyr_set_in(in, pd->scanner);

    int result = tenyr_parse(pd);
    if (!result && f)
        assembly_inner(pd, out, f);
    tenyr_lex_destroy(pd->scanner);

    return result;
}

int do_disassembly(FILE *in, FILE *out, const struct format *f)
{
    struct instruction i;
    void *ud;
    if (f->init)
        f->init(in, ASM_DISASSEMBLE, &ud);

    uint32_t reladdr = 0;
    while (f->in(in, &i, ud) == 1) {
        int len = print_disassembly(out, &i, ASM_AS_INSN);
        fprintf(out, "%*s# ", 30 - len, "");
        print_disassembly(out, &i, ASM_AS_DATA);
        fprintf(out, " ; ");
        print_disassembly(out, &i, ASM_AS_CHAR);
        // TODO make i.reladdr correct so we can use that XXX hack
        fprintf(out, " ; .addr 0x%06x\n", reladdr++); //i.reladdr);
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

