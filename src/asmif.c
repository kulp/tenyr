#include "asmif.h"
#include "ops.h"
// obj.h is included for RLC_* flags ; reconsider their location
#include "obj.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"
#include "common.h"
#include "asm.h"

#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>

#if _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#if EMSCRIPTEN
// needed for EMCC so far
void *lfind(const void *key, const void *base, size_t *nmemb, size_t size,
        int(*compar)(const void *, const void *));
#endif

enum { RHS_FLIP = 1 << 0, NO_NAMED_RELOC = 1 << 1 };

static const char shortopts[] = "df:o:sv" "hV";
static const struct option longopts[] = {
    { "disassemble" ,       no_argument, NULL, 'd' },
    { "format"      , required_argument, NULL, 'f' },
    { "output"      , required_argument, NULL, 'o' },
    { "strict"      ,       no_argument, NULL, 's' },
    { "verbose"     ,       no_argument, NULL, 'v' },

    { "help"        ,       no_argument, NULL, 'h' },
    { "version"     ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

#define version() "tas version " STR(BUILD_NAME)

typedef int reloc_handler(struct parse_data *pd, struct element *evalctx,
        struct element *defctx, int flags, struct const_expr *ce, void
        *ud);

static int ce_eval(struct parse_data *pd, struct element *evalctx, struct
        element *defctx, struct const_expr *ce, int flags, reloc_handler
        *rhandler, void *rud, uint32_t *result);

static int add_relocation(struct parse_data *pd, const char *name,
        struct element *insn, int width, int flags);

struct symbol *symbol_find(struct symbol_list *list, const char *name)
{
    list_foreach(symbol_list, elt, list)
        if (!strncmp(elt->symbol->name, name, SYMBOL_LEN))
            return elt->symbol;

    return NULL;
}

// symbol_lookup returns 1 on success
static int symbol_lookup(struct parse_data *pd, struct symbol_list *list, const
        char *name, uint32_t *result)
{
    struct symbol *symbol = NULL;
    if ((symbol = symbol_find(list, name))) {
        if (result) {
            if (symbol->ce) {
                struct element_list **prev = symbol->ce->deferred;
                struct element *c = (prev && *prev) ? (*prev)->elem : NULL;
                return ce_eval(pd, c, NULL, symbol->ce, 0, NULL, NULL, result);
            } else {
                *result = symbol->reladdr;
            }
        }
        return 1;
    }

    // unresolved symbols get a zero value, but this is still success in CE_EXT
    // case (not in CE_SYM case)
    if (result) *result = 0;
    return 0;
}

// add_relocation returns 1 on success
static int add_relocation(struct parse_data *pd, const char *name,
        struct element *insn, int width, int flags)
{
    struct reloc_list *node = calloc(1, sizeof *node);

    strcopy(node->reloc.name, name ? name : "", sizeof node->reloc.name);
    node->reloc.insn  = insn;
    node->reloc.width = width;
    node->reloc.flags = flags;

    node->next = pd->relocs;
    pd->relocs = node;

    if (insn)
        insn->reloc = &node->reloc;

    return 1;
}

static int sym_reloc_handler(struct parse_data *pd, struct element
        *evalctx, struct element *defctx, int flags, struct const_expr *ce,
        void *ud)
{
    int rc = 0;
    int *width = ud;

    int rlc_flags = 0;

    if (flags & RHS_FLIP)
        rlc_flags |= RLC_NEGATE;

    switch (ce->type) {
        case CE_SYM:
        case CE_EXT:
            if (ce->symbol && ce->symbol->ce) {
                return defctx ? add_relocation(pd, NULL, defctx, *width, rlc_flags) : 0;
            } else if (ce->type == CE_EXT) {
                const char *name = ce->symbol ? ce->symbol->name : ce->symbolname;
                const char *n = (flags & NO_NAMED_RELOC) ? NULL : name;
                return add_relocation(pd, n, evalctx, *width, rlc_flags);
            }
        case CE_ICI:
            return add_relocation(pd, NULL, evalctx, *width, rlc_flags);
        default:
            return 0;
    }

    return rc;
}

// ce_eval should be idempotent. returns 1 on fully-successful evaluation, 0 on incomplete evaluation
static int ce_eval(struct parse_data *pd, struct element *evalctx, struct
        element *defctx, struct const_expr *ce, int flags, reloc_handler
        *rhandler, void *rud, uint32_t *result)
{
    uint32_t left, right;

    switch (ce->type) {
        case CE_SYM:
        case CE_EXT:
            if (ce->symbol && ce->symbol->ce) {
                struct element *dc = (*ce->symbol->ce->deferred)->elem;
                return ce_eval(pd, evalctx, dc, ce->symbol->ce, flags, rhandler, rud, result)
                    || (rhandler ? rhandler(pd, evalctx, dc, flags, ce, rud) : 0);
            } else {
                const char *name = ce->symbol ? ce->symbol->name : ce->symbolname;
                int found = symbol_lookup(pd, pd->symbols, name, result);
                int hflags = flags;
                if (found)
                    hflags |= NO_NAMED_RELOC;
                int handled = (rhandler ? rhandler(pd, evalctx, defctx, hflags, ce, rud) : 0);
                return found || handled;
            }
        case CE_ICI:
            // XXX explain this voodoo ; why should defctx and evalctx work this way ?
            *result = defctx ? defctx->insn.reladdr : evalctx ? evalctx->insn.reladdr : 0;
            return rhandler ? rhandler(pd, evalctx, defctx, flags, ce, rud) : !!defctx;
        case CE_IMM: *result = ce->i; return 1;
        case CE_OP2: {
            int lhsflags = flags;
            int rhsflags = flags;
            if (ce->op == '-')
                rhsflags ^= RHS_FLIP;
            // TODO what if rhandler doesn't always succeed ? could change lhs but not rhs
            if (ce_eval(pd, evalctx, defctx, ce->left , lhsflags, rhandler, rud, &left) &&
                ce_eval(pd, evalctx, defctx, ce->right, rhsflags, rhandler, rud, &right))
            {
                switch (ce->op) {
                    case '+': *result = left +  right; return 1;
                    case '-': *result = left -  right; return 1;
                    case '*': *result = left *  right; return 1;
                    case '^': *result = left ^  right; return 1;
                    case '/': *result = left /  right; return 1;
                    case LSH: *result = left << right; return 1;
                    default: fatal(0, "Unrecognised const_expr op '%c'", ce->op);
                }
            }
            return 0;
        }
        default:
            fatal(0, "Unrecognised const_expr type %d", ce->type);
            return 0;
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
        if (ce_eval(pd, ce->insn, NULL, ce, 0, sym_reloc_handler, &r->width, &result)) {
            uint32_t mask = ~((1ULL << r->width) - 1);
            result *= r->mult;

            int hasupperbits = result & mask;
            uint32_t semask = -1 << (r->width - 1);
            int notsignextended = (result & semask) != semask;

            if (hasupperbits && notsignextended) {
                debug(0, "Expression resulting in value %#x is too large for "
                        "%d-bit signed immediate field", result, r->width);
                rc |= 1;
            }

            *r->dest &= mask;
            *r->dest |= result & ~mask;
            ce_free(ce, 1);
        } else {
            fatal(0, "Error while fixing up deferred expressions");
            // TODO print out information about the deferred expression
        }

        free(r);
    }

    return rc;
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
    list_foreach(element_list, Node, pd->top) {
        free(Node->elem);
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
    list_foreach(element_list, il, pd->top) {
        if (!il->elem)
            continue;

        il->elem->insn.reladdr = reladdr;

        list_foreach(symbol, l, il->elem->symbol) {
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
                if (ce_eval(pd, NULL, NULL, l->ce, 0, sym_reloc_handler,
                            (int[]){ WORD_BITWIDTH }, &l->reladdr))
                    l->resolved = 1;

    return 0;
}

static int assembly_inner(struct parse_data *pd, FILE *out, const struct format *f)
{
    assembly_fixup_insns(pd);

    mark_globals(pd->symbols, pd->globals);
    if (check_symbols(pd->symbols))
        fatal(0, "Error in symbol processing : check for duplicate symbols");

    if (!fixup_deferred_exprs(pd)) {
        void *ud;
        if (f->init)
            f->init(out, ASM_ASSEMBLE, &ud);

        list_foreach(element_list, Node, pd->top)
            // if !Node->elem, it's a placeholder or some kind of dummy
            if (Node->elem)
                f->out(out, Node->elem, ud);

        if (f->sym)
            list_foreach(symbol_list, Node, pd->symbols)
                f->sym(out, Node->symbol, ud);

        if (f->reloc)
            list_foreach(reloc_list, Node, pd->relocs)
                f->reloc(out, &Node->reloc, ud);

        if (f->fini)
            f->fini(out, &ud);
    } else {
        fatal(0, "Error while fixing up deferred expressions");
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
    if (pd->errored)
        fatal(0, "Encountered %d error%s while parsing, bailing",
                pd->errored, &"s"[pd->errored == 1]);
    if (!result && f)
        assembly_inner(pd, out, f);
    tenyr_lex_destroy(pd->scanner);

    return result;
}

int do_disassembly(FILE *in, FILE *out, const struct format *f, int flags)
{
    int rc = 0;

    struct element i;
    void *ud = NULL;
    if (f->init)
        f->init(in, ASM_DISASSEMBLE, &ud);

    uint32_t reladdr = 0;
    while ((rc = f->in(in, &i, ud)) == 1) {
        int len = print_disassembly(out, &i, ASM_AS_INSN | flags);
        if (!(flags & ASM_QUIET)) {
            fprintf(out, "%*s# ", 30 - len, "");
            print_disassembly(out, &i, ASM_AS_DATA | flags);
            fprintf(out, " ; ");
            print_disassembly(out, &i, ASM_AS_CHAR | flags);
            // TODO make i.reladdr correct so we can use that XXX hack
            fprintf(out, " ; .addr 0x%06x\n", reladdr++); //i.reladdr);
        } else {
            fputc('\n', out);
            // This probably means we want line-oriented output
            fflush(out);
        }
    }

    rc = feof(in) ? 0 : -1;

    if (f->fini)
        rc |= f->fini(in, &ud);

    fflush(out);

    return rc;
}

