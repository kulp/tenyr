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

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s);
static struct const_expr *add_deferred_expr(struct parse_data *pd, struct
        const_expr *ce, int mult, uint32_t *dest, int width);
static struct const_expr *make_const_expr(enum const_expr_type type, int op,
        struct const_expr *left, struct const_expr *right, int flags);
static struct expr *make_expr_type0(int x, int op, int y, int mult, struct
        const_expr *defexpr);
static struct expr *make_expr_type1(int x, int op, struct const_expr *defexpr,
        int y);
static struct expr *make_unary_type0(int x, int op, int mult, struct const_expr
        *defexpr);
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
static int check_immediate_size(struct parse_data *pd, YYLTYPE *locp, uint32_t
        imm);

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
%left "==" "<>"
%left '<' '>'
%left "<<" ">>"
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
%token WORD ASCII UTF32 GLOBAL SET

/* synonyms for literal string tokens */
%token LSH "<<"
%token RSH ">>"
%token EQ "=="
%token NEQ "<>"
%token XORN "^~"
%token ANDN "&~"

%type <ce> const_expr pconst_expr preloc_expr greloc_expr
%type <ce> reloc_expr const_atom eref here_atom here_expr phere_expr here
%type <cl> reloc_expr_list
%type <cstr> string
%type <dctv> directive
%type <expr> rhs_plain rhs_deref lhs_plain lhs_deref
%type <i> arrow regname reloc_op const_op
%type <imm> immediate
%type <insn> insn insn_inner
%type <op> op unary_op
%type <program> program ascii utf32 data string_or_data
%type <str> symbol symbol_list

%expect 3

%union {
    int32_t i;
    uint32_t u;
    struct const_expr *ce;
    struct const_expr_list *cl;
    struct expr *expr;
    struct cstr *cstr;
    struct directive *dctv;
    struct instruction *insn;
    struct instruction_list *program;
    struct immediate {
        int32_t i;
        int is_bits;    ///< if this immediate should be treated as a bitstring
    } imm;
    struct strbuf {
        int pos, len;
        char buf[LINE_LEN];
    } str;
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
    | symbol ':' opt_nl string_or_data[inner]
        {   $outer = $inner;
            struct symbol *n = add_symbol_to_insn(&yyloc, $inner->insn,
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
        {   struct instruction_list *p = $string_or_data, *i = $inner;
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
            $outer->insn = $insn; }

sep
    : '\n'
    | ';'

insn[outer]
    : ILLEGAL
        {   $outer = calloc(1, sizeof *$outer);
            $outer->u.word = -1; }
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
    | lhs_plain tor rhs_plain
        {   struct expr *t0 = make_expr_type0($rhs_plain->x, OP_BITWISE_OR, 0, 0, NULL),
                        *t1 = make_expr_type0($lhs_plain->x, OP_BITWISE_OR, 0, 0, NULL);
            $insn_inner = make_insn_general(pd, t0, 0, t1);
            free(t1);
            free(t0);
            free($rhs_plain);
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
    :   /* empty */
        {   $outer = NULL; }
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
    : WORD reloc_expr_list
        {   tenyr_pop_state(pd->scanner); $data = make_data(pd, $reloc_expr_list); }

directive
    : GLOBAL symbol_list
        {   tenyr_pop_state(pd->scanner); $directive = make_directive(pd, &yylloc, D_GLOBAL, $symbol_list); }
    | SET SYMBOL ',' reloc_expr
        {   tenyr_pop_state(pd->scanner); $directive = make_directive(pd, &yylloc, D_SET, $SYMBOL.buf, $reloc_expr); }

symbol_list
    : SYMBOL /* TODO permit comma-separated symbol lists for GLOBAL */

reloc_expr_list[outer]
    : reloc_expr[expr]
        {   $outer = calloc(1, sizeof *$outer);
            $outer->right = NULL;
            $outer->ce = $expr; }
    | reloc_expr[expr] ',' reloc_expr_list[inner]
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
    /* type0 */
    : regname[x] op regname[y] reloc_op greloc_expr
        { $rhs_plain = make_expr_type0($x, $op, $y, $reloc_op == '+' ? 1 : -1, $greloc_expr); }
    | regname[x] op regname[y]
        { $rhs_plain = make_expr_type0($x, $op, $y, 0, NULL); }
    | regname[x]
        { $rhs_plain = make_expr_type0($x, OP_BITWISE_OR, 0, 0, NULL); }
    | unary_op regname[x] reloc_op greloc_expr
        { $rhs_plain = make_unary_type0($x, $unary_op, $reloc_op == '+' ? 1 : -1, $greloc_expr); }
    | unary_op regname[x]
        { $rhs_plain = make_unary_type0($x, $unary_op, 0, NULL); }
    | greloc_expr
        {   /* When the RHS is just a constant, choose OR or ADD depending on
             * what kind of constant this is likely to be. */
            enum op op = ($greloc_expr->flags & IMM_IS_BITS) ? OP_BITWISE_OR : OP_ADD;
            $rhs_plain = make_expr_type0(0, op, 0, 1, $greloc_expr); }
    /* type1 */
    | regname[x] op greloc_expr '+' regname[y]
        { $rhs_plain = make_expr_type1($x, $op, $greloc_expr, $y); }
    | regname[x] op greloc_expr
        { $rhs_plain = make_expr_type1($x, $op, $greloc_expr, 0); }
    | unary_op greloc_expr /* responsible for a S/R conflict */
        { $rhs_plain = make_expr_type1(0, $unary_op, $greloc_expr, 0); }
    | greloc_expr '+' regname[y]
        { $rhs_plain = make_expr_type1(0, OP_ADD, $greloc_expr, $y); }

unary_op
    : '~' { $unary_op = OP_BITWISE_XORN; }
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
    | '-' INTEGER
        {   $immediate.i = -$INTEGER;
            $immediate.is_bits = 0; }
    | BITSTRING
        {   $immediate.i = $BITSTRING;
            $immediate.is_bits = 1; }
    | CHARACTER
        {   $immediate.i = $CHARACTER.buf[$CHARACTER.pos - 1];
            $immediate.is_bits = 1; }

op
    : '+'   { $op = OP_ADD                ; }
    | '-'   { $op = OP_SUBTRACT           ; }
    | '*'   { $op = OP_MULTIPLY           ; }
    | '<'   { $op = OP_COMPARE_LT         ; }
    | "=="  { $op = OP_COMPARE_EQ         ; }
    | '>'   { $op = OP_COMPARE_GT         ; }
    | "<>"  { $op = OP_COMPARE_NE         ; }
    | '|'   { $op = OP_BITWISE_OR         ; }
    | '&'   { $op = OP_BITWISE_AND        ; }
    | "&~"  { $op = OP_BITWISE_ANDN       ; }
    | '^'   { $op = OP_BITWISE_XOR        ; }
    | "^~"  { $op = OP_BITWISE_XORN       ; }
    | "<<"  { $op = OP_SHIFT_LEFT         ; }
    | ">>"  { $op = OP_SHIFT_RIGHT_LOGICAL; }


arrow
    : tol { $arrow = 0; }
    | tor { $arrow = 1; }

tol : TOL { tenyr_pop_state(pd->scanner); }
tor : TOR { tenyr_pop_state(pd->scanner); }

reloc_op
    : '+' { $reloc_op = '+'; }
    | '-' { $reloc_op = '-'; }

/* guarded reloc_exprs : either a single term, or a parenthesised reloc_expr */
greloc_expr
    : eref
    | here_atom
    | preloc_expr
    | const_atom
        {   struct const_expr *c = $const_atom;
            if (c->type == CE_IMM)
                check_immediate_size(pd, &yylloc, c->i);
            $greloc_expr = c;
        }

reloc_expr[outer]
    : const_expr
    | eref
    | preloc_expr
    | here_expr
    | eref reloc_op const_atom
        {   $outer = make_const_expr(CE_OP2, $reloc_op, $eref, $const_atom, 0); }
    | eref reloc_op here_atom
        {   $outer = make_const_expr(CE_OP2, $reloc_op, $eref, $here_atom, 0); }
    | eref reloc_op[lop] here_atom reloc_op[rop] const_atom
        {   struct const_expr *inner = make_const_expr(CE_OP2, $lop, $eref, $here_atom, 0);
            $outer = make_const_expr(CE_OP2, $rop, inner, $const_atom, 0);
        }

const_op
    : reloc_op
    | '*'  { $const_op = '*'; }
    | '/'  { $const_op = '/'; }
    | '^'  { $const_op = '^'; }
    | "<<" { $const_op = LSH; }

here_atom
    : here
    | phere_expr

here
    : '.'
        {   $here = make_const_expr(CE_ICI, 0, NULL, NULL, IMM_IS_BITS); }

here_expr[outer]
    : here_atom
    | here_expr[left] const_op const_atom[right]
        {   $outer = make_const_expr(CE_OP2, $const_op, $left, $right, 0); }

phere_expr
    : '(' here_expr ')'
        {   $phere_expr = $here_expr; }

const_expr[outer]
    : const_atom
    | const_expr[left] const_op const_atom[right]
        {   $outer = make_const_expr(CE_OP2, $const_op, $left, $right, 0); }

const_atom
    : pconst_expr
    | immediate
        {   $const_atom = make_const_expr(CE_IMM, 0, NULL, NULL, $immediate.is_bits ? IMM_IS_BITS : 0);
            $const_atom->i = $immediate.i; }
    | LOCAL
        {   $const_atom = make_const_expr(CE_SYM, 0, NULL, NULL, IMM_IS_BITS);
            struct symbol *s;
            if ((s = symbol_find(pd->symbols, $LOCAL.buf))) {
                $const_atom->symbol = s;
            } else {
                strcopy($const_atom->symbolname, $LOCAL.buf, sizeof $const_atom->symbolname);
            }
        }

preloc_expr
    : '(' reloc_expr ')'
        {   $preloc_expr = $reloc_expr; }

pconst_expr
    : '(' const_expr ')'
        {   $pconst_expr = $const_expr; }

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
    struct deferred_expr *n = calloc(1, sizeof *n);

    n->next  = pd->defexprs;
    n->ce    = ce;
    n->dest  = dest;
    n->width = width;
    n->mult  = mult;

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

static struct expr *make_expr_type0(int x, int op, int y, int mult, struct
        const_expr *defexpr)
{
    struct expr *e = calloc(1, sizeof *e);

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
    struct expr *e = calloc(1, sizeof *e);

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

static struct expr *make_unary_type0(int x, int op, int mult, struct const_expr
        *defexpr)
{
    // tenyr has no true unary ops, but the following sugars are recognised by
    // the assembler and converted into their corresponding binary equivalents :
    //
    // b <- ~b      becomes     b <- b ^~ a
    // b <- -b      becomes     b <- a -  b

    struct expr *e = calloc(1, sizeof *e);

    switch (op) {
        case OP_SUBTRACT:
            e->x = 0;
            e->y = x;
            break;
        case OP_BITWISE_XORN:
            e->x = x;
            e->y = 0;
            break;
    }

    e->type  = 0;
    e->deref = 0;
    e->op    = op;
    e->mult  = mult;
    e->ce    = defexpr;
    if (defexpr)
        e->i = 0xfffffbad; // put in a placeholder that must be overwritten
    else
        e->i = 0; // there was no const_expr ; zero defined by language

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
            *rp = calloc(1, sizeof **rp);
            (*rp)->prev = t;
            t = *rp;
            rp = &t->next;
            t->insn = calloc(1, sizeof *t->insn);

            t->insn->u.word = p->str[spos];
        }

        p = p->right;
    }

    free_cstr(cs, 1);

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
                *rp = calloc(1, sizeof **rp);
                (*rp)->prev = t;
                t = *rp;
                rp = &t->next;
                t->insn = calloc(1, sizeof *t->insn);
            }

            t->insn->u.word |= (p->str[spos] & 0xff) << ((wpos % 4) * 8);
        }

        p = p->right;
    }

    free_cstr(cs, 1);

    return result;
}

static struct symbol *add_symbol_to_insn(YYLTYPE *locp, struct instruction *insn, const char *symbol)
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

static struct instruction_list *make_data(struct parse_data *pd, struct const_expr_list *list)
{
    struct instruction_list *result = NULL, **rp = &result, *last = NULL;

    struct const_expr_list *p = list;
    while (p) {
        *rp = calloc(1, sizeof **rp);
        struct instruction_list *q = *rp;
        q->prev = last;
        last = *rp;
        rp = &q->next;

        q->insn = calloc(1, sizeof *q->insn);
        add_deferred_expr(pd, p->ce, 1, &q->insn->u.word, WORD_BITWIDTH);
        p->ce->insn = q->insn;
        struct const_expr_list *temp = p;
        p = p->right;
        free(temp);
    }

    return result;
}

struct datum_D_SET {
    struct symbol *symbol;
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
            const struct strbuf symbol = va_arg(vl,const struct strbuf);
            result->data = malloc(symbol.len + 1);
            strcopy(result->data, symbol.buf, symbol.len + 1);
            break;
        case D_SET: {
            result->type = type;
            // TODO stop allocating datum_D_SET if we don't need it
            struct datum_D_SET *d = result->data = malloc(sizeof *d);
            const struct strbuf symbol = va_arg(vl,const struct strbuf);

            struct symbol *n = calloc(1, sizeof *n);
            n->column   = locp->first_column;
            n->lineno   = locp->first_line;
            n->resolved = 0;
            n->next     = NULL;
            n->ce       = va_arg(vl,struct const_expr *);
            n->unique   = 0;
            if (symbol.len > sizeof n->name) {
                tenyr_error(locp, pd, "symbol too long in .set directive");
                return NULL;
            }
            strcopy(n->name, symbol.buf, symbol.len + 1);

            d->symbol = n;

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
            strcopy(g->name, d->data, sizeof g->name);
            free(d->data);
            g->next = pd->globals;
            pd->globals = g;
            free(d);
            break;
        }
        case D_SET: {
            struct datum_D_SET *data = d->data;
            struct instruction_list **context = NULL;

            // XXX this deferral code is broken
            if (!p->insn)
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

static int check_immediate_size(struct parse_data *pd, YYLTYPE *locp, uint32_t
        imm)
{
    int hasupperbits = imm & ~SMALL_IMMEDIATE_MASK;
    uint32_t semask = -1 << (SMALL_IMMEDIATE_BITWIDTH - 1);
    int notsignextended = (imm & semask) != semask;

    if (hasupperbits && notsignextended) {
        char buf[128];
        snprintf(buf, sizeof buf, "Immediate with value %#x is too large for "
                "%d-bit signed immediate field", imm, SMALL_IMMEDIATE_BITWIDTH);
        tenyr_error(locp, pd, buf);

        return 1;
    }

    return 0;
}

