%{
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "parser_global.h"
#include "parser.h"
#include "lexer.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s);
static struct const_expr *add_deferred_expr(struct parse_data *pd, struct
        const_expr *ce, int mult, uint32_t *dest, int width);
static struct const_expr *make_const_expr(enum const_expr_type type, int op,
        struct const_expr *left, struct const_expr *right);
static struct expr *make_expr_type0(int x, int op, int y, int mult, struct
        const_expr *defexpr);
static struct expr *make_expr_type1(int x, int op, struct const_expr *defexpr,
        int y);
static struct instruction *make_insn_general(struct parse_data *pd, struct
        expr *lhs, int arrow, struct expr *expr);
static struct instruction_list *make_ascii(struct cstr *cs);
static struct instruction_list *make_utf32(struct cstr *cs);
static struct symbol *add_symbol_to_insn(YYLTYPE *locp, struct instruction *insn,
        const char *symbol);
static int add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n);
static int check_add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n);
static struct instruction_list *make_data(struct parse_data *pd, struct
        const_expr_list *list);
static struct directive *make_directive(struct parse_data *pd, YYLTYPE *locp,
        enum directive_type type, ...);
static void handle_directive(struct parse_data *pd, YYLTYPE *locp, struct
        directive *d, struct instruction_list *p);

#define YYLEX_PARAM (pd->scanner)

void ce_free(struct const_expr *ce, int recurse);
struct symbol *symbol_find(struct symbol_list *list, const char *name);

%}

%error-verbose
%pure-parser
%locations
%define parse.lac full
%lex-param { void *yyscanner }
%parse-param { struct parse_data *pd }
%name-prefix "tenyr_"

%start top

%left EQ NEQ
%left LTE '>'
%left '+' '-'
%left '*'
%left '^' XORN
%left '|' NOR
%left '&' NAND
%left LSH RSH

%token <chr> '[' ']' '.' '(' ')'
%token <chr> '+' '-' '*'
%token <chr> ','
%token <arrow> TOL TOR
%token <str> INTEGER SYMBOL LOCAL STRING
%token <chr> REGISTER
%token ILLEGAL
%token WORD ASCII UTF32 GLOBAL SET

%type <ce> const_expr pconst_expr preloc_expr greloc_expr reloc_expr const_atom eref
%type <cl> reloc_expr_list
%type <expr> rhs rhs_plain rhs_plain_type0 rhs_plain_type1 rhs_deref lhs_plain lhs_deref
%type <i> arrow immediate regname reloc_op
%type <insn> insn insn_inner
%type <op> op
%type <program> program ascii utf32 data string_or_data
%type <s> addsub
%type <str> symbol
%type <cstr> string
%type <dctv> directive

%union {
    int32_t i;
    signed s;
    struct const_expr *ce;
    struct const_expr_list *cl;
    struct expr *expr;
    struct cstr *cstr;
    struct directive *dctv;
    struct instruction *insn;
    struct instruction_list *program;
    char str[LINE_LEN];
    char chr;
    int op;
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
    | symbol ':' string_or_data[inner]
        {   $outer = $inner;
            struct symbol *n = add_symbol_to_insn(&yyloc, $inner->insn, $symbol);
            if (check_add_symbol(&yyloc, pd, n))
                YYABORT;
        }

program[outer]
    :   /* empty */
        {   $outer = NULL; }
    | string_or_data program[inner]
        {   struct instruction_list *p = $string_or_data;
            while (p->next) p = p->next;
            p->next = $inner;

            $outer = malloc(sizeof *$outer);
            $outer->next = $string_or_data->next;
            $outer->insn = $string_or_data->insn;
            $inner->prev = $outer;
            free($string_or_data); }
    | directive program[inner]
        {   $outer = $inner;
            $directive->follows = &$inner->prev;
            $directive->precedes = &$inner;
            handle_directive(pd, &yylloc, $directive, $inner); }
    | insn program[inner]
        {   $outer = malloc(sizeof *$outer);
            $inner->prev = $outer;
            $outer->next = $inner;
            $outer->insn = $insn; }

insn[outer]
    : ILLEGAL
        {   $outer = calloc(1, sizeof *$outer);
            $outer->u.word = -1; }
    | insn_inner
    | symbol ':' insn[inner]
        {   $outer = $inner;
            struct symbol *n = add_symbol_to_insn(&yyloc, $inner, $symbol);
            if (check_add_symbol(&yyloc, pd, n))
                YYABORT;
        }

insn_inner
    : lhs_plain arrow rhs
        {   $insn_inner = make_insn_general(pd, $lhs_plain, $arrow, $rhs);
            free($rhs);
            free($lhs_plain); }
    | lhs_deref arrow rhs_plain
        {   $insn_inner = make_insn_general(pd, $lhs_deref, $arrow, $rhs_plain);
            free($rhs_plain);
            free($lhs_deref); }

string[outer]
    :   /* empty */
        {   $outer = NULL; }
    | STRING string[inner]
        {   $outer = calloc(1, sizeof *$outer);
            $outer->len = strlen($STRING) - 2; // drop quotes
            $outer->str = malloc($outer->len + 1);
            // skip quotes
            strncpy($outer->str, $STRING + 1, $outer->len);
            $outer->right = $inner; }

utf32
    : UTF32 string
        {   $utf32 = make_utf32($string); }

ascii
    : ASCII string
        {   $ascii = make_ascii($string); }

data
    : WORD reloc_expr_list
        {   $data = make_data(pd, $reloc_expr_list); }

directive
    : GLOBAL SYMBOL
        {   $directive = make_directive(pd, &yylloc, D_GLOBAL, $SYMBOL); }
    | SET SYMBOL ',' reloc_expr
        {   $directive = make_directive(pd, &yylloc, D_SET, $SYMBOL, $reloc_expr); }

reloc_expr_list[outer]
    : reloc_expr[expr]
        {   $outer = calloc(1, sizeof $outer);
            $outer->right = NULL;
            $outer->ce = $expr; }
    | reloc_expr[expr] ',' reloc_expr_list[inner]
        {   $outer = calloc(1, sizeof $outer);
            $outer->right = $inner;
            $outer->ce = $expr; }

lhs_plain
    : regname
        {   ($lhs_plain = malloc(sizeof *$lhs_plain))->x = $regname;
            $lhs_plain->deref = 0; }

lhs_deref
    : '[' lhs_plain ']'
        {   $lhs_deref = $lhs_plain;
            $lhs_deref->deref = 1; }

rhs
    : rhs_plain
    | rhs_deref

rhs_plain
    : rhs_plain_type0
    | rhs_plain_type1

rhs_plain_type0
    : regname[x] op regname[y] addsub greloc_expr
        { $rhs_plain_type0 = make_expr_type0($x, $op, $y, $addsub, $greloc_expr); }
    | regname[x] op regname[y]
        { $rhs_plain_type0 = make_expr_type0($x, $op, $y, 0, NULL); }
    | regname[x]
        { $rhs_plain_type0 = make_expr_type0($x, OP_BITWISE_OR, 0, 0, NULL); }

rhs_plain_type1
    : regname[x] op greloc_expr addsub regname[y]
        { $rhs_plain_type1 = make_expr_type1($x, $op, $greloc_expr, $y); }
    | regname[x] op greloc_expr
        { $rhs_plain_type1 = make_expr_type1($x, $op, $greloc_expr, 0); }
    | greloc_expr
        { $rhs_plain_type1 = make_expr_type1(0, OP_BITWISE_OR, $greloc_expr, 0); }

rhs_deref
    : '[' rhs_plain ']'
        {   $rhs_deref = $rhs_plain;
            $rhs_deref->deref = 1; }

regname
    : REGISTER { $regname = toupper($REGISTER) - 'A'; }

immediate
    : INTEGER { $immediate = strtoll($INTEGER, NULL, 0); }

addsub
    : '+' { $addsub =  1; }
    | '-' { $addsub = -1; }

op
    : '|'   { $op = OP_BITWISE_OR         ; }
    | '&'   { $op = OP_BITWISE_AND        ; }
    | '+'   { $op = OP_ADD                ; }
    | '*'   { $op = OP_MULTIPLY           ; }
    | LSH   { $op = OP_SHIFT_LEFT         ; }
    | LTE   { $op = OP_COMPARE_LTE        ; }
    | EQ    { $op = OP_COMPARE_EQ         ; }
    | NOR   { $op = OP_BITWISE_NOR        ; }
    | NAND  { $op = OP_BITWISE_NAND       ; }
    | '^'   { $op = OP_BITWISE_XOR        ; }
    | '-'   { $op = OP_ADD_NEGATIVE_Y     ; }
    | XORN  { $op = OP_XOR_INVERT_X       ; }
    | RSH   { $op = OP_SHIFT_RIGHT_LOGICAL; }
    | '>'   { $op = OP_COMPARE_GT         ; }
    | NEQ   { $op = OP_COMPARE_NE         ; }

arrow
    : TOL { $arrow = 0; }
    | TOR { $arrow = 1; }

reloc_op
    : '+' { $$ = '+'; }
    | '-' { $$ = '-'; }

/* guarded reloc_expr : either a single term, or a parenthesised reloc_expr */
greloc_expr
    : eref
    | const_atom
    | preloc_expr

reloc_expr[outer]
    : const_expr
    | eref
    | preloc_expr
    | eref reloc_op const_atom
        {   $outer = make_const_expr(OP2, $reloc_op, $eref, $const_atom); }

const_expr[outer]
    : const_atom
    | const_expr[left] reloc_op const_atom[right]
        {   $outer = make_const_expr(OP2, $reloc_op, $left, $right); }
    | const_expr[left] '*' const_atom[right]
        {   $outer = make_const_expr(OP2, '*', $left, $right); }
    | const_expr[left] LSH const_atom[right]
        {   $outer = make_const_expr(OP2, LSH, $left, $right); }

const_atom
    : pconst_expr
    | immediate
        {   $const_atom = make_const_expr(IMM, 0, NULL, NULL);
            $const_atom->i = $immediate; }
    | LOCAL
        {   $const_atom = make_const_expr(SYM, 0, NULL, NULL);
            struct symbol *s;
            if ((s = symbol_find(pd->symbols, $LOCAL))) {
                $const_atom->symbol = s;
            } else {
                strncpy($const_atom->symbolname, $LOCAL, sizeof $const_atom->symbolname);
                $const_atom->symbolname[sizeof $const_atom->symbolname - 1] = 0;
            }
        }
    | '.'
        {   $const_atom = make_const_expr(ICI, 0, NULL, NULL); }

preloc_expr
    : '(' reloc_expr ')'
        {   $preloc_expr = $reloc_expr; }

pconst_expr
    : '(' const_expr ')'
        {   $pconst_expr = $const_expr; }

eref
    : '@' SYMBOL
        {   $eref = make_const_expr(EXT, 0, NULL, NULL);
            struct symbol *s;
            if ((s = symbol_find(pd->symbols, $SYMBOL))) {
                $eref->symbol = s;
            } else {
                strncpy($eref->symbolname, $SYMBOL, sizeof $eref->symbolname);
                $eref->symbolname[sizeof $eref->symbolname - 1] = 0;
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

    return 0;
}

static struct instruction *make_insn_general(struct parse_data *pd, struct
        expr *lhs, int arrow, struct expr *expr)
{
    struct instruction *insn = calloc(1, sizeof *insn);

    int dd = ((lhs->deref | !!arrow) << 1) | expr->deref;

    insn->u._0xxx.t   = 0;
    insn->u._0xxx.p   = expr->type;
    insn->u._0xxx.dd  = dd;
    insn->u._0xxx.z   = lhs->x;
    insn->u._0xxx.x   = expr->x;
    insn->u._0xxx.op  = expr->op;
    insn->u._0xxx.y   = expr->y;
    insn->u._0xxx.imm = expr->i;

    if (expr->ce) {
        add_deferred_expr(pd, expr->ce, expr->mult, &insn->u.word,
                SMALL_IMMEDIATE_BITWIDTH);
        expr->ce->insn = insn;
    }

    return insn;
}

static struct const_expr *add_deferred_expr(struct parse_data *pd, struct
        const_expr *ce, int mult, uint32_t *dest, int width)
{
    struct deferred_expr *n = malloc(sizeof *n);

    n->next  = pd->defexprs;
    n->ce    = ce;
    n->dest  = dest;
    n->width = width;
    n->mult  = mult;

    pd->defexprs = n;

    return ce;
}

static struct const_expr *make_const_expr(enum const_expr_type type, int op,
        struct const_expr *left, struct const_expr *right)
{
    struct const_expr *n = malloc(sizeof *n);

    n->type  = type;
    n->op    = op;
    n->left  = left;
    n->right = right;
    n->insn  = NULL;    // top const_expr will have its insn filled in

    return n;
}

static struct expr *make_expr_type0(int x, int op, int y, int mult, struct
        const_expr *defexpr)
{
    struct expr *e = malloc(sizeof *e);

    e->type  = 0;
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

static struct expr *make_expr_type1(int x, int op, struct const_expr *defexpr,
        int y)
{
    struct expr *e = malloc(sizeof *e);

    e->type  = 1;
    e->deref = 0;
    e->x     = x;
    e->op    = op;
    e->ce    = defexpr;
    e->mult  = 1;
    e->y     = y;
    if (defexpr)
        e->i = 0xfffffbad; // put in a placeholder that must be overwritten
    else
        e->i = 0; // there was no const_expr ; zero defined by language

    return e;
}

static struct instruction_list *make_utf32(struct cstr *cs)
{
    struct instruction_list *result = NULL, **rp = &result;

    struct cstr *p = cs;
    unsigned wpos = 0; // position in the word
    struct instruction_list *t = *rp;

    while (p) {
        unsigned spos = 0; // position in the string
        int len = p->len;
        for (; len > 0; wpos++, spos++, len--) {
            struct instruction_list *temp = *rp;
            *rp = calloc(1, sizeof **rp);
            t = *rp;
            t->next = temp;
            rp = &t->next;
            t->insn = calloc(1, sizeof *t->insn);

            t->insn->u.word = p->str[spos];
        }

        p = p->right;
    }

    return result;
}

static struct instruction_list *make_ascii(struct cstr *cs)
{
    struct instruction_list *result = NULL, **rp = &result;

    struct cstr *p = cs;
    unsigned wpos = 0; // position in the word
    struct instruction_list *t = *rp;
    while (p) {
        unsigned spos = 0; // position in the string
        int len = p->len;
        for (; len > 0; wpos++, spos++, len--) {
            if (wpos % 4 == 0) {
                struct instruction_list *temp = *rp;
                *rp = calloc(1, sizeof **rp);
                t = *rp;
                t->next = temp;
                rp = &t->next;
                t->insn = calloc(1, sizeof *t->insn);
            }

            t->insn->u.word |= (p->str[spos] & 0xff) << ((wpos % 4) * 8);
        }

        p = p->right;
    }

    return result;
}

static struct symbol *add_symbol_to_insn(YYLTYPE *locp, struct instruction *insn, const char *symbol)
{
    struct symbol *n = calloc(1, sizeof *n);
    n->column   = locp->first_column;
    n->lineno   = locp->first_line;
    n->resolved = 0;
    n->next     = insn->symbol;
    strncpy(n->name, symbol, sizeof n->name);
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
    // TODO check if it exists already
    if (symbol_find(pd->symbols, n->name)) {
        char buf[128];
        snprintf(buf, sizeof buf, "Error adding symbol '%s' (already exists ?)", n->name);
        tenyr_error(locp, pd, buf);
        return 1;
    } else
        return add_symbol(locp, pd, n);
}

static struct instruction_list *make_data(struct parse_data *pd, struct const_expr_list *list)
{
    struct instruction_list *result = NULL, **rp = &result;

    struct const_expr_list *p = list;
    while (p) {
        *rp = calloc(1, sizeof **rp);
        struct instruction_list *q = *rp;
        rp = &q->next;

        q->insn = calloc(1, sizeof *q->insn);
        add_deferred_expr(pd, p->ce, 1, &q->insn->u.word, WORD_BITWIDTH);
        p->ce->insn = q->insn;
        p = p->right;
    }

    return result;
}

struct datum_D_SET {
    char name[SYMBOL_LEN];
    struct const_expr *ce;
};

static struct directive *make_directive(struct parse_data *pd, YYLTYPE *locp,
        enum directive_type type, ...)
{
    struct directive *result = calloc(1, sizeof *result);
    result->type = D_NULL;

    va_list vl;
    va_start(vl,type);

    switch (type) {
        case D_GLOBAL:
            result->type = type;
            result->data = malloc(SYMBOL_LEN);
            const char *symbol = va_arg(vl,const char *);
            snprintf(result->data, SYMBOL_LEN, symbol);
            break;
        case D_SET: {
            result->type = type;
            // TODO stop allocating datum_D_SET if we don't need it
            struct datum_D_SET *d = result->data = malloc(sizeof *d);
            const char *symbol = va_arg(vl,const char *);
            snprintf(result->data, SYMBOL_LEN, symbol);

            struct symbol *n = calloc(1, sizeof *n);
            n->column   = locp->first_column;
            n->lineno   = locp->first_line;
            n->resolved = 0;
            n->next     = NULL;
            n->ce       = va_arg(vl,struct const_expr *);
            strncpy(n->name, symbol, sizeof n->name);

            add_symbol(locp, pd, n);
            break;
        }
        default: {
            char buf[128];
            snprintf(buf, sizeof buf, "Unknown directive type %d in %s", type, __func__);
            tenyr_error(locp, pd, buf);
        }
    }

    va_end(vl);
    return result;
}

static void handle_directive(struct parse_data *pd, YYLTYPE *locp, struct
        directive *d, struct instruction_list *p)
{
    switch (d->type) {
        case D_GLOBAL: {
            struct global_list *g = malloc(sizeof *g);
            strncpy(g->name, d->data, sizeof g->name);
            free(d->data);
            g->next = pd->globals;
            pd->globals = g;
            free(d);
            break;
        }
        case D_SET: {
            struct datum_D_SET *data = d->data;

            free(data);
            break;
        }
        default: {
            char buf[128];
            snprintf(buf, sizeof buf, "Unknown directive type %d in %s", d->type, __func__);
            tenyr_error(locp, pd, buf);
        }
    }
}

