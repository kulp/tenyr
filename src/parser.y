%{
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "parser_global.h"
#include "parser.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s);
static struct const_expr *add_deferred_expr(struct parse_data *pd, struct
        const_expr *ce, int mult, uint32_t *dest, int width, int flags);
static struct const_expr *make_const_expr(enum const_expr_type type, int op,
        struct const_expr *left, struct const_expr *right, int flags);
static struct expr *make_expr(int type, int x, int op, int y, int mult, struct
        const_expr *defexpr);
static struct expr *make_unary(int op, int x, int y, int mult, struct
        const_expr *defexpr);
static struct element *make_insn_general(struct parse_data *pd, struct
        expr *lhs, int arrow, struct expr *expr);
static struct element_list *make_ascii(struct cstr *cs);
static struct element_list *make_utf32(struct cstr *cs);
static struct symbol *add_symbol_to_insn(YYLTYPE *locp, struct element *insn,
        const char *symbol);
static int add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n);
static int check_add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n);
static struct element_list *make_data(struct parse_data *pd, struct
        const_expr_list *list);
static struct element_list *make_zeros(struct parse_data *pd, struct
        const_expr *size);
static struct directive *make_global(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *sym);
static struct directive *make_set(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *sym, struct const_expr *expr);
static void handle_directive(struct parse_data *pd, YYLTYPE *locp, struct
        directive *d, struct element_list *p);

#define YYLEX_PARAM (pd->scanner)

struct symbol *symbol_find(struct symbol_list *list, const char *name);

extern void tenyr_push_state(int st, void *yyscanner);
extern void tenyr_pop_state(void *yyscanner);
%}

%error-verbose
%pure-parser
%locations
%define parse.lac full
%lex-param { void *yyscanner }
/* declare parse_data struct as opaque for bison 2.6 */
%code requires { struct parse_data; }
%code {
    #define YY_HEADER_EXPORT_START_CONDITIONS 1
    #include "lexer.h"
}
%parse-param { struct parse_data *pd }
%name-prefix "tenyr_"

%start top

/* precedence rules only matter in constant expressions */
%left '|'
%left '^' "^~"
%left '&' "&~"
%left "==" '<' '>' "<=" ">="
%left "<<" ">>" ">>>"
%left '+' '-'
%left '*' '/'

%token <chr> '[' ']' '.' '(' ')'
%token <chr> '+' '-' '*' '~'
%token <chr> ',' ';'
%token <arrow> TOL TOR
%token <str> SYMBOL LOCAL STRING CHARACTER
%token <i> INTEGER BITSTRING
%token <chr> REGISTER
%token ILLEGAL
%token WORD ASCII UTF32 GLOBAL SET ZERO

/* synonyms for literal string tokens */
%token LSH "<<"
%token RSH ">>"
%token RSHA ">>>"
%token EQ "=="
%token GE ">="
%token LE "<="
%token ORN "|~"
%token ANDN "&~"
%token PACK "^^"

%type <ce> const_expr greloc_expr reloc_expr_atom
%type <ce> reloc_expr here_or_const_atom const_atom eref here_atom here_expr
%type <cl> reloc_expr_list
%type <cstr> string
%type <dctv> directive
%type <expr> rhs_plain rhs_sugared rhs_deref lhs_plain lhs_deref
%type <i> arrow regname reloc_op const_op
%type <imm> immediate
%type <insn> insn insn_inner
%type <op> native_op sugar_op unary_op reloc_unary_op
%type <program> program ascii utf32 data string_or_data
%type <str> symbol symbol_list

%expect 2

%union {
    int32_t i;
    uint32_t u;
    struct const_expr *ce;
    struct const_expr_list *cl;
    struct expr *expr;
    struct cstr *cstr;
    struct directive *dctv;
    struct element *insn;
    struct element_list *program;
    struct immediate {
        int32_t i;
        int is_bits;    ///< if this immediate should be treated as a bitstring
    } imm;
    struct strbuf {
        int pos, len;
        char buf[LINE_LEN];
    } str;
    char chr;
    signed op; ///< negative ops are used to mark swapped operands
    int arrow;
}

%%

top
    : program
        {   pd->top = $program; }

string_or_data[outer]
    : ascii
    | utf32
    | data
    | symbol ':' opt_nl string_or_data[inner]
        {   $outer = $inner;
            struct symbol *n = add_symbol_to_insn(&yyloc, $inner->elem,
                    $symbol.buf);
            if (check_add_symbol(&yyloc, pd, n))
                YYABORT;
        }

opt_nl
    : '\n' opt_nl
    | /* empty */

program[outer]
    :   /* empty */
        {   $outer = calloc(1, sizeof *$outer);
            // dummy instruction permits capturing previous instruction from $outer->prev
        }
    | sep program[inner]
        {   $outer = $inner; }
    | string_or_data sep program[inner]
        {   struct element_list *p = $string_or_data, *i = $inner;
            if (p) {
                while (p->next) p = p->next;
                p->next = i;
                i->prev = p;
                $outer = $string_or_data;
            } else
                $outer = i; // an empty string is NULL
        }
    | directive sep program[inner]
        {   $outer = $inner;
            handle_directive(pd, &yylloc, $directive, $inner); }
    | insn sep program[inner]
        {   $outer = calloc(1, sizeof *$outer);
            $inner->prev = $outer;
            $outer->next = $inner;
            $outer->elem = $insn; }

sep
    : '\n'
    | ';'

insn[outer]
    : ILLEGAL
        {   $outer = calloc(1, sizeof *$outer);
            $outer->insn.size = 1;
            $outer->insn.u.word = 0xcfffffff; /* P <- -1 */ }
    | insn_inner
    | symbol ':' opt_nl insn[inner]
        {   $outer = $inner;
            struct symbol *n = add_symbol_to_insn(&yyloc, $inner, $symbol.buf);
            if (check_add_symbol(&yyloc, pd, n))
                YYABORT;
        }

insn_inner
    : lhs_plain tol rhs_plain
        {   $insn_inner = make_insn_general(pd, $lhs_plain, 0, $rhs_plain);
            free($rhs_plain);
            free($lhs_plain); }
    | lhs_plain tor regname[x]
        {   struct expr *t0 = make_expr(0, $x, OP_BITWISE_OR, 0, 0, NULL),
                        *t1 = make_expr(0, $lhs_plain->x, OP_BITWISE_OR, 0, 0, NULL);
            $insn_inner = make_insn_general(pd, t0, 0, t1);
            free(t1);
            free(t0);
            free($lhs_plain); }
    | lhs_plain arrow rhs_deref
        {   $insn_inner = make_insn_general(pd, $lhs_plain, $arrow, $rhs_deref);
            free($rhs_deref);
            free($lhs_plain); }
    | lhs_deref arrow rhs_plain
        {   $insn_inner = make_insn_general(pd, $lhs_deref, $arrow, $rhs_plain);
            free($rhs_plain);
            free($lhs_deref); }

string[outer]
    : STRING
        {   $outer = calloc(1, sizeof *$outer);
            $outer->len = $STRING.len;
            $outer->str = malloc($outer->len + 1);
            strcopy($outer->str, $STRING.buf, $outer->len + 1); }
    | STRING string[inner]
        {   $outer = calloc(1, sizeof *$outer);
            $outer->len = $STRING.len;
            $outer->str = malloc($outer->len + 1);
            strcopy($outer->str, $STRING.buf, $outer->len + 1);
            $outer->right = $inner; }

utf32
    : UTF32 string
        {   tenyr_pop_state(pd->scanner); $utf32 = make_utf32($string); }

ascii
    : ASCII string
        {   tenyr_pop_state(pd->scanner); $ascii = make_ascii($string); }

data
    : WORD opt_nl reloc_expr_list
        {   tenyr_pop_state(pd->scanner); $data = make_data(pd, $reloc_expr_list); }
    | ZERO opt_nl reloc_expr
        {   tenyr_pop_state(pd->scanner); $data = make_zeros(pd, $reloc_expr); }

directive
    : GLOBAL opt_nl symbol_list
        {   tenyr_pop_state(pd->scanner); $directive = make_global(pd, &yylloc, &$symbol_list); }
    | SET opt_nl SYMBOL ',' reloc_expr
        {   tenyr_pop_state(pd->scanner); $directive = make_set(pd, &yylloc, &$SYMBOL, $reloc_expr); }

symbol_list
    : SYMBOL /* TODO permit comma-separated symbol lists for GLOBAL */

reloc_expr_list[outer]
    : reloc_expr[expr]
        {   $outer = calloc(1, sizeof *$outer);
            $outer->right = NULL;
            $outer->ce = $expr; }
    | reloc_expr[expr] ',' opt_nl reloc_expr_list[inner]
        {   $outer = calloc(1, sizeof *$outer);
            $outer->right = $inner;
            $outer->ce = $expr; }

lhs_plain
    : regname
        {   /* this isn't semantically the ideal place to start looking for
               arrows, but it works */
            tenyr_push_state(needarrow, pd->scanner);
            ($lhs_plain = malloc(sizeof *$lhs_plain))->x = $regname;
            $lhs_plain->deref = 0; }

lhs_deref
    : '[' lhs_plain ']'
        {   $lhs_deref = $lhs_plain;
            $lhs_deref->deref = 1; }

rhs_plain
    /* syntax sugars */
    : rhs_sugared
    /* type0 */
    | regname[x] native_op regname[y] reloc_op greloc_expr
        { $rhs_plain = make_expr(0, $x, $native_op, $y, $reloc_op == '+' ? 1 : -1, $greloc_expr); }
    | regname[x] native_op regname[y]
        { $rhs_plain = make_expr(0, $x, $native_op, $y, 0, NULL); }
    | regname[x]
        { $rhs_plain = make_expr(0, $x, OP_BITWISE_OR, 0, 0, NULL); }
    | regname[x] sugar_op regname[y] '+' greloc_expr
        { $rhs_plain = make_expr(0, $x, -$sugar_op, $y, 1, $greloc_expr); }
    | regname[x] sugar_op regname[y]
        { $rhs_plain = make_expr(0, $x, -$sugar_op, $y, 0, NULL); }
    /* type1 */
    | regname[x] native_op greloc_expr '+' regname[y]
        { $rhs_plain = make_expr(1, $x, $native_op, $y, 1, $greloc_expr); }
    | regname[x] native_op greloc_expr
        { $rhs_plain = make_expr(1, $x, $native_op, 0, 1, $greloc_expr); }
    | greloc_expr sugar_op regname[x] '+' regname[y]
        { $rhs_plain = make_expr(1, $x, -$sugar_op, $y, 1, $greloc_expr); }
    | greloc_expr sugar_op regname[x]
        { $rhs_plain = make_expr(1, $x, -$sugar_op, 0, 1, $greloc_expr); }
    /* type2 */
    | greloc_expr native_op regname[x] '+' regname[y]
        { $rhs_plain = make_expr(2, $x, $native_op, $y, 1, $greloc_expr); }
    | greloc_expr native_op regname[x]
        { $rhs_plain = make_expr(2, $x, $native_op, 0, 1, $greloc_expr); }
    | regname[x] sugar_op greloc_expr '+' regname[y]
        { $rhs_plain = make_expr(2, $x, -$sugar_op, $y, 1, $greloc_expr); }
    | regname[x] sugar_op greloc_expr
        { $rhs_plain = make_expr(2, $x, -$sugar_op, 0, 1, $greloc_expr); }
    /* type3 */
    | greloc_expr
        {   struct const_expr *ce = $greloc_expr;
            int is_bits = ce->flags & IMM_IS_BITS;
            /* Large immediates and ones that should be expressed in
             * hexadecimal use type3 ; others use type0. */
            if (is_bits || ce->i != SEXTEND32(SMALL_IMMEDIATE_BITWIDTH,ce->i)) {
                $rhs_plain = make_expr(3, 0, OP_BITWISE_OR, 0, 1, ce);
            } else {
                $rhs_plain = make_expr(0, 0, OP_ADD, 0, 1, ce);
            }
        }

rhs_sugared
    : unary_op regname[x]
        { $rhs_sugared = make_unary($unary_op, $x,  0, 0, NULL); }
    | unary_op regname[x] '+' regname[y]
        { $rhs_sugared = make_unary($unary_op, $x, $y, 0, NULL); }
    | unary_op regname[x] reloc_op greloc_expr
        { $rhs_sugared = make_unary($unary_op, $x,  0, $reloc_op == '+' ? 1 : -1, $greloc_expr); }

unary_op
    : '~' { $unary_op = OP_BITWISE_ORN; }
    | '-' { $unary_op = OP_SUBTRACT; }

rhs_deref
    : '[' rhs_plain ']'
        {   $rhs_deref = $rhs_plain;
            $rhs_deref->deref = 1; }

regname
    : REGISTER { $regname = toupper($REGISTER) - 'A'; }

immediate
    : INTEGER
        {   $immediate.i = $INTEGER;
            $immediate.is_bits = 0; }
    | BITSTRING
        {   $immediate.i = $BITSTRING;
            $immediate.is_bits = 1; }
    | CHARACTER
        {   $immediate.i = $CHARACTER.buf[$CHARACTER.pos - 1];
            $immediate.is_bits = 1; }

native_op
    : '+'   { $native_op = OP_ADD              ; }
    | '-'   { $native_op = OP_SUBTRACT         ; }
    | '*'   { $native_op = OP_MULTIPLY         ; }
    | '<'   { $native_op = OP_COMPARE_LT       ; }
    | "=="  { $native_op = OP_COMPARE_EQ       ; }
    | ">="  { $native_op = OP_COMPARE_GE       ; }
    | "<>"  { $native_op = OP_COMPARE_NE       ; }
    | '|'   { $native_op = OP_BITWISE_OR       ; }
    | "|~"  { $native_op = OP_BITWISE_ORN      ; }
    | '&'   { $native_op = OP_BITWISE_AND      ; }
    | "&~"  { $native_op = OP_BITWISE_ANDN     ; }
    | '^'   { $native_op = OP_BITWISE_XOR      ; }
    | "<<"  { $native_op = OP_SHIFT_LEFT       ; }
    | ">>"  { $native_op = OP_SHIFT_RIGHT_LOGIC; }
    | ">>>" { $native_op = OP_SHIFT_RIGHT_ARITH; }
    | "^^"  { $native_op = OP_PACK             ; }

sugar_op
    : '>'   { $sugar_op = -OP_COMPARE_LT; }
    | "<="  { $sugar_op = -OP_COMPARE_GE; }

arrow
    : tol { $arrow = 0; }
    | tor { $arrow = 1; }

tol : TOL { tenyr_pop_state(pd->scanner); }
tor : TOR { tenyr_pop_state(pd->scanner); }

reloc_op
    : '+' { $reloc_op = '+'; }
    | '-' { $reloc_op = '-'; }

reloc_unary_op
    : '~' { $reloc_unary_op = '~'; }
    | '-' { $reloc_unary_op = '-'; }

/* guarded reloc_exprs : either a single term, or a parenthesised reloc_expr */
greloc_expr
    : reloc_expr_atom
    | here_or_const_atom

reloc_expr[outer]
    : const_expr
    | reloc_expr_atom
    | here_expr
    | eref reloc_op here_or_const_atom
        {   $outer = make_const_expr(CE_OP2, $reloc_op, $eref, $here_or_const_atom, 0); }
    | eref reloc_op[lop] here_atom reloc_op[rop] const_atom
        {   struct const_expr *inner = make_const_expr(CE_OP2, $lop, $eref, $here_atom, 0);
            $outer = make_const_expr(CE_OP2, $rop, inner, $const_atom, 0);
        }

here_or_const_atom
    : here_atom
    | const_atom

reloc_expr_atom
    : eref
    | '(' reloc_expr ')'
        {   $reloc_expr_atom = $reloc_expr; }

const_op
    : reloc_op
    | '*'  { $const_op = '*'; }
    | '/'  { $const_op = '/'; }
    | '^'  { $const_op = '^'; }
    | "<<" { $const_op = LSH; }

here_atom
    : '.'
        {   $here_atom = make_const_expr(CE_ICI, 0, NULL, NULL, IMM_IS_BITS); }
    | '(' here_expr ')'
        {   $here_atom = $here_expr; }

here_expr[outer]
    : here_atom
    | here_expr[left] const_op const_atom[right]
        {   $outer = make_const_expr(CE_OP2, $const_op, $left, $right, 0); }

const_expr[outer]
    : const_atom
    | const_expr[left] const_op const_atom[right]
        {   $outer = make_const_expr(CE_OP2, $const_op, $left, $right, 0); }

const_atom[outer]
    : reloc_unary_op const_atom[inner]
        {   $outer = make_const_expr(CE_OP1, $reloc_unary_op, $inner, NULL, 0); }
    | '(' const_expr ')'
        {   $outer = $const_expr; }
    | immediate
        {   $outer = make_const_expr(CE_IMM, 0, NULL, NULL, $immediate.is_bits ? IMM_IS_BITS : 0);
            $outer->i = $immediate.i; }
    | LOCAL
        {   $outer = make_const_expr(CE_SYM, 0, NULL, NULL, IMM_IS_BITS);
            struct symbol *s;
            if ((s = symbol_find(pd->symbols, $LOCAL.buf))) {
                $outer->symbol = s;
            } else {
                strcopy($outer->symbolname, $LOCAL.buf, sizeof $outer->symbolname);
            }
        }

eref
    : '@' SYMBOL
        {   $eref = make_const_expr(CE_EXT, 0, NULL, NULL, IMM_IS_BITS);
            struct symbol *s;
            if ((s = symbol_find(pd->symbols, $SYMBOL.buf))) {
                $eref->symbol = s;
            } else {
                strcopy($eref->symbolname, $SYMBOL.buf, sizeof $eref->symbolname);
            }
        }

symbol
    : SYMBOL
    | LOCAL

%%

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s)
{
    fflush(stderr);
    fprintf(stderr, "%s\n", pd->lexstate.saveline);
    fprintf(stderr, "%*s\n%*s at line %d column %d at `%s'\n",
            locp->first_column, "^", locp->first_column, s,
            locp->first_line, locp->first_column,
            tenyr_get_text(pd->scanner));

    pd->errored++;
    return 0;
}

static struct element *make_insn_general(struct parse_data *pd, struct
        expr *lhs, int arrow, struct expr *expr)
{
    struct element *elem = calloc(1, sizeof *elem);

    int dd = ((lhs->deref | !!arrow) << 1) | expr->deref;

    elem->insn.u.typeany.p  = expr->type;
    elem->insn.u.typeany.dd = dd;
    elem->insn.u.typeany.z  = lhs->x;
    elem->insn.u.typeany.x  = expr->x;

    switch (expr->type) {
        case 0:
        case 1:
        case 2:
            elem->insn.u.type012.op  = expr->op;
            elem->insn.u.type012.y   = expr->y;
            elem->insn.u.type012.imm = expr->i;
            break;
        case 3:
            elem->insn.u.type3.imm = expr->i;
    }

    elem->insn.size = 1;

    if (expr->ce) {
        int width = (expr->type == 3)
            ? MEDIUM_IMMEDIATE_BITWIDTH
            : SMALL_IMMEDIATE_BITWIDTH;
        add_deferred_expr(pd, expr->ce, expr->mult, &elem->insn.u.word,
                width, 0);
        expr->ce->insn = elem;
    }

    return elem;
}

static struct const_expr *add_deferred_expr(struct parse_data *pd, struct
        const_expr *ce, int mult, uint32_t *dest, int width, int flags)
{
    struct deferred_expr *n = calloc(1, sizeof *n);

    n->next  = pd->defexprs;
    n->ce    = ce;
    n->dest  = dest;
    n->width = width;
    n->mult  = mult;
    n->flags = flags;

    pd->defexprs = n;

    return ce;
}

static struct const_expr *make_const_expr(enum const_expr_type type, int op,
        struct const_expr *left, struct const_expr *right, int flags)
{
    struct const_expr *n = calloc(1, sizeof *n);

    n->type  = type;
    n->op    = op;
    n->left  = left;
    n->right = right;
    n->insn  = NULL;    // top const_expr will have its insn filled in
    n->flags = (left  ? left->flags  : 0) |
               (right ? right->flags : 0) | flags;

    return n;
}

static struct expr *make_expr(int type, int x, int op, int y, int mult, struct
        const_expr *defexpr)
{
    struct expr *e = calloc(1, sizeof *e);

    e->type  = type;
    e->deref = 0;
    e->x     = x;
    e->op    = op;
    e->y     = y;
    e->mult  = mult;
    e->ce    = defexpr;
    if (defexpr)
        e->i = 0xfffffbad; // put in a placeholder that must be overwritten
    else
        e->i = 0; // there was no const_expr ; zero defined by language

    return e;
}

static struct expr *make_unary(int op, int x, int y, int mult, struct
        const_expr *defexpr)
{
    // tenyr has no true unary ops, but the following sugars are recognised by
    // the assembler and converted into their corresponding binary equivalents :
    //
    // b <- ~b + 2  becomes     b <- a |~ b + 2 (type 0)
    // b <- ~b + c  becomes     b <- 0 |~ b + c (type 2)
    // b <- -b + 2  becomes     b <- a -  b + 2 (type 0)
    // b <- -b + c  becomes     b <- 0 -  b + c (type 2)

    struct expr *e = calloc(1, sizeof *e);

    e->deref = 0;
    e->op    = op;
    e->mult  = mult;

    if (defexpr) {
        e->type = 0;
        e->x = 0;
        e->y = x;
        e->ce = defexpr;
        e->i = 0xfffffbad; // put in a placeholder that must be overwritten
    } else {
        e->type = 2;
        e->x = x;
        e->y = y;
        e->i = 0; // there was no const_expr ; zero defined by language
    }

    return e;
}

static void free_cstr(struct cstr *cs, int recurse)
{
    if (!cs)
        return;

    if (recurse)
        free_cstr(cs->right, recurse);

    free(cs->str);
    free(cs);
}

static struct element_list *make_utf32(struct cstr *cs)
{
    struct element_list *result = NULL, **rp = &result;

    struct cstr *p = cs;
    unsigned wpos = 0; // position in the word
    struct element_list *t = *rp;

    while (p) {
        unsigned spos = 0; // position in the string
        int len = p->len;
        for (; len > 0; wpos++, spos++, len--) {
            *rp = calloc(1, sizeof **rp);
            (*rp)->prev = t;
            t = *rp;
            rp = &t->next;
            t->elem = calloc(1, sizeof *t->elem);

            t->elem->insn.u.word = p->str[spos];
            t->elem->insn.size = 1;
        }

        p = p->right;
    }

    free_cstr(cs, 1);

    return result;
}

static struct element_list *make_ascii(struct cstr *cs)
{
    struct element_list *result = NULL, **rp = &result;

    struct cstr *p = cs;
    unsigned wpos = 0; // position in the word
    struct element_list *t = *rp;
    while (p) {
        unsigned spos = 0; // position in the string
        int len = p->len;
        for (; len > 0; wpos++, spos++, len--) {
            if (wpos % 4 == 0) {
                *rp = calloc(1, sizeof **rp);
                (*rp)->prev = t;
                t = *rp;
                rp = &t->next;
                t->elem = calloc(1, sizeof *t->elem);
            }

            t->elem->insn.u.word |= (p->str[spos] & 0xff) << ((wpos % 4) * 8);
            t->elem->insn.size = 1;
        }

        p = p->right;
    }

    free_cstr(cs, 1);

    return result;
}

static struct symbol *add_symbol_to_insn(YYLTYPE *locp, struct element *insn, const char *symbol)
{
    struct symbol *n = calloc(1, sizeof *n);
    n->column   = locp->first_column;
    n->lineno   = locp->first_line;
    n->resolved = 0;
    n->next     = insn->symbol;
    n->unique   = 1;
    strcopy(n->name, symbol, sizeof n->name);
    insn->symbol = n;

    return n;
}

static int add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n)
{
    struct symbol_list *l = calloc(1, sizeof *l);

    l->next  = pd->symbols;
    l->symbol = n;
    pd->symbols = l;

    return 0;
}

static int check_add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n)
{
    // TODO we could check for colliding symbols at parse time, but I don't
    // believe this is reliable right now. It would be much nicer for
    // diagnostics, though.
    #if CHECK_SYMBOLS_DURING_PARSE
    if (symbol_find(pd->symbols, n->name)) {
        char buf[128];
        snprintf(buf, sizeof buf, "Error adding symbol '%s' (already exists ?)", n->name);
        tenyr_error(locp, pd, buf);
        return 1;
    } else
    #endif
        return add_symbol(locp, pd, n);
}

static struct element_list *make_data(struct parse_data *pd, struct const_expr_list *list)
{
    struct element_list *result = NULL, **rp = &result, *last = NULL;

    struct const_expr_list *p = list;
    while (p) {
        *rp = calloc(1, sizeof **rp);
        struct element_list *q = *rp;
        q->prev = last;
        last = *rp;
        rp = &q->next;

        q->elem = calloc(1, sizeof *q->elem);
        q->elem->insn.size = 1;
        add_deferred_expr(pd, p->ce, 1, &q->elem->insn.u.word, WORD_BITWIDTH, 0);
        p->ce->insn = q->elem;
        struct const_expr_list *temp = p;
        p = p->right;
        free(temp);
    }

    return result;
}

static struct element_list *make_zeros(struct parse_data *pd, struct const_expr *size)
{
    struct element_list *result = calloc(1, sizeof *result);
    result->elem = calloc(1, sizeof *result->elem);
    add_deferred_expr(pd, size, 1, &result->elem->insn.size, WORD_BITWIDTH, CE_SIMPLE);
    return result;
}

struct datum_D_SET {
    struct symbol *symbol;
};

static struct directive *make_global(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *symbol)
{
    struct directive *result = calloc(1, sizeof *result);
    result->type = D_GLOBAL;
    result->data = malloc(symbol->len + 1);
    strcopy(result->data, symbol->buf, symbol->len + 1);
    return result;
}

static struct directive *make_set(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *symbol, struct const_expr *expr)
{
    struct directive *result = calloc(1, sizeof *result);
    result->type = D_SET;
    // TODO stop allocating datum_D_SET if we don't need it
    struct datum_D_SET *d = result->data = malloc(sizeof *d);

    struct symbol *n = calloc(1, sizeof *n);
    n->column   = locp->first_column;
    n->lineno   = locp->first_line;
    n->resolved = 0;
    n->next     = NULL;
    n->ce       = expr;
    n->unique   = 0;
    if (symbol->len > sizeof n->name) {
        tenyr_error(locp, pd, "symbol too long in .set directive");
        return NULL;
    }
    strcopy(n->name, symbol->buf, symbol->len + 1);

    d->symbol = n;

    add_symbol(locp, pd, n);

    return result;
}

static void handle_directive(struct parse_data *pd, YYLTYPE *locp, struct
        directive *d, struct element_list *p)
{
    switch (d->type) {
        case D_GLOBAL: {
            struct global_list *g = malloc(sizeof *g);
            strcopy(g->name, d->data, sizeof g->name);
            free(d->data);
            g->next = pd->globals;
            pd->globals = g;
            free(d);
            break;
        }
        case D_SET: {
            struct datum_D_SET *data = d->data;
            struct element_list **context = NULL;

            // XXX this deferral code is broken
            if (!p->elem)
                context = &p->prev; // dummy instruction at end ; defer to prev
            else if (p->next)
                context = &p->next->prev; // otherwise, defer to current instruction node
            else
                fatal(0, "Illegal instruction context for .set");

            data->symbol->ce->deferred = context;
            free(data);
            free(d);
            break;
        }
        default: {
            char buf[128];
            snprintf(buf, sizeof buf, "Unknown directive type %d in %s", d->type, __func__);
            tenyr_error(locp, pd, buf);
        }
    }
}

