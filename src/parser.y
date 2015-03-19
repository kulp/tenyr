%{
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common.h"
#include "parser_global.h"
#include "parser.h"
#include "asm.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *fmt, ...);
static struct const_expr *add_deferred_expr(struct parse_data *pd,
        struct const_expr *ce, int mult, uint32_t *dest, int width, int flags);
static struct const_expr *make_expr(struct parse_data *pd, YYLTYPE *locp,
        enum const_expr_type type, int op, struct const_expr *left,
        struct const_expr *right, int flags);
static struct expr *make_rhs(int type, int x, int op, int y, int mult,
        struct const_expr *defexpr);
static struct expr *make_unary(int op, int x, int y, int mult,
        struct const_expr *defexpr);
static struct element *make_insn_general(struct parse_data *pd,
        struct expr *lhs, int arrow, struct expr *expr);
static struct element_list *make_utf32(struct cstr *cs);
static int add_symbol_to_insn(struct parse_data *pd, YYLTYPE *locp,
        struct element *insn, struct cstr *symbol);
static struct element_list *make_data(struct parse_data *pd,
        struct const_expr_list *list);
static struct element_list *make_zeros(struct parse_data *pd, YYLTYPE *locp,
        struct const_expr *size);
static struct directive *make_global(struct parse_data *pd, YYLTYPE *locp,
        const struct cstr *sym);
static struct directive *make_set(struct parse_data *pd, YYLTYPE *locp,
        const struct cstr *sym, struct const_expr *expr);
static void handle_directive(struct parse_data *pd, YYLTYPE *locp,
        struct directive *d, struct element_list **context);
static int is_type3(struct const_expr *ce);
static struct const_expr *make_ref(struct parse_data *pd, YYLTYPE *locp,
        int type, const struct cstr *symbol);
static struct const_expr_list *make_expr_list(struct const_expr *expr,
        struct const_expr_list *right);
static int validate_expr(struct parse_data *pd, struct const_expr *e, int level);

// XXX decide whether this should be called in functions or in grammar actions
static void free_cstr(struct cstr *cs, int recurse);
extern struct symbol *symbol_find(struct symbol_list *list, const char *name);
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
    #define yyscanner (pd->scanner)
    #define PUSH(State) tenyr_push_state(State, yyscanner)
    #define POP         tenyr_pop_state(yyscanner)
    #define ERR(...)    tenyr_error(&yylloc, pd, __VA_ARGS__)
}
%parse-param { struct parse_data *pd }
%name-prefix "tenyr_"

%start top

/* precedence rules only matter in constant expressions */
%left           '|'
%left           '^'
%left           '&' "&~"
%left           "==" '<' '>' "<=" ">="
%left           "<<" ">>" ">>>"
%left           '+' '-'
%left           '*' '/'

%token <i>      '[' ']' '.' '(' ')' '+' '-' '*' '~' ',' ';'
%token <i>      INTEGER BITSTRING REGISTER
%token <cstr>   SYMBOL LOCAL STRSPAN CHARACTER
%token          ILLEGAL
%token          OPNQUOTE CLSQUOTE

/* synonyms for literal string tokens */
%token LSH      "<<"
%token RSH      ">>>"
%token RSHA     ">>"
%token EQ       "=="
%token GE       ">="
%token LE       "<="
%token ORN      "|~"
%token ANDN     "&~"
%token PACK     "^^"

%token <i> TOL  "<-"
%token <i> TOR  "->"

%token WORD     ".word"
%token UTF32    ".utf32"
%token GLOBAL   ".global"
%token SET      ".set"
%token ZERO     ".zero"

%type <ce>      binop_expr expr vexpr atom vatom eref
%type <cl>      expr_list
%type <cstr>    string stringelt strspan symbol
%type <dctv>    directive
%type <expr>    rhs rhs_plain lhs lhs_plain
%type <i>       arrow regname reloc_op binop unary_op const_unary_op
%type <imm>     immediate
%type <program> program element data insn

%union {
    int32_t                 i;
    struct const_expr      *ce;
    struct const_expr_list *cl;
    struct expr            *expr;
    struct cstr            *cstr;
    struct directive       *dctv;
    struct element_list    *program;
    struct immediate {
        int32_t i;
        int flags;
    } imm;
}

%%

top
    : opt_terminators
        {   pd->top = NULL; }
    | opt_terminators program
        {   // Allocate a phantom entry to provide a final context element
            *(pd->top ? &pd->top->tail->next : &pd->top) =
                calloc(1, sizeof *pd->top); }

opt_nl
    : opt_nl '\n'
    | /* empty */

program
    : element terminators
        {   pd->top = $$ = $1; }
    | program element terminators
        {   struct element_list *p = $1, *d = $element;
            if (p == NULL) {
                p = d;
            } else if (d != NULL) { // an empty string is NULL
                p->tail->next = d;
                p->tail = d->tail;
            }
            pd->top = $$ = p;
        }
    | directive terminators
        {   pd->top = $$ = NULL;
            handle_directive(pd, &yylloc, $directive, &pd->top); }
    | program directive terminators
        {   handle_directive(pd, &yylloc, $directive,
                             pd->top ? &pd->top->tail->next : &pd->top); }

element
    : data
    | insn
    | symbol ':' opt_nl element[inner]
        {   add_symbol_to_insn(pd, &yylloc, ($$ = $inner)->elem, $symbol);
            free_cstr($symbol, 1); }

opt_terminators : /* empty */ | terminators
terminator  : '\n' | ';'
terminators : terminators terminator | terminator

insn
    : ILLEGAL
        {   $$ = calloc(1, sizeof *$$);
            $$->elem = calloc(1, sizeof *$$->elem);
            $$->elem->insn.size = 1;
            $$->elem->insn.u.word = 0xffffffff; /* P <- [P + -1] */
            $$->tail = $$; }
    | lhs { PUSH(needarrow); } arrow { POP; } rhs
        {   if ($lhs->deref && $rhs->deref)
                ERR("Cannot dereference both sides of an arrow");
            if ($arrow == 1 && !$rhs->deref)
                ERR("Right arrows must point to dereferenced right-hand sides");
            $$ = calloc(1, sizeof *$$);
            $$->elem = make_insn_general(pd, $lhs, $arrow, $rhs);
            $$->tail = $$; }

symbol
    : SYMBOL
    | LOCAL

string
    : stringelt
    | string[left] stringelt
        {   $$ = $left;
            $$->last->right = $stringelt;
            if ($stringelt) $$->last = $stringelt; }

stringelt
    : OPNQUOTE strspan CLSQUOTE {   $$ = $strspan; }
    | OPNQUOTE CLSQUOTE         {   $$ = NULL; }

strspan
    : STRSPAN
    | strspan[left] STRSPAN
        {   $$ = $left;
            $$->last->right = $STRSPAN;
            $$->last = $$->last->right; }

data
    : ".word"  opt_nl expr_list {  POP; $$ = make_data(pd, $expr_list); }
    | ".zero"  opt_nl expr      {  POP; $$ = make_zeros(pd, &yylloc, $expr); }
    | ".utf32" opt_nl string    {  POP; $$ = make_utf32($string); free_cstr($string, 1); }

directive
    : ".global" opt_nl SYMBOL
        {   POP; $directive = make_global(pd, &yylloc, $SYMBOL); free_cstr($SYMBOL, 1); }
    | ".set" opt_nl SYMBOL ',' expr
        {   POP; $directive = make_set(pd, &yylloc, $SYMBOL, $expr); free_cstr($SYMBOL, 1); }

expr_list
    : vexpr
        {   $$ = make_expr_list($vexpr, NULL); }
    | vexpr ',' opt_nl expr_list[right]
        {   $$ = make_expr_list($vexpr, $right); }

lhs
    : lhs_plain
    | '[' lhs_plain ']'
        {   $$ = $lhs_plain;
            $$->deref = 1; }

lhs_plain
    : regname
        {   $$ = calloc(1, sizeof *$$);
            $$->x = $regname; }

rhs
    : rhs_plain
    | '[' rhs_plain ']'
        {   $$ = $rhs_plain;
            $$->deref = 1; }

rhs_plain
    /* type0 */
    : regname[x] binop regname[y] reloc_op vatom
        { $$ = make_rhs(0, $x, $binop, $y, $reloc_op, $vatom); }
    | regname[x] binop regname[y]
        { $$ = make_rhs(0, $x, $binop, $y, 0, NULL); }
    | regname[x]
        { $$ = make_rhs(0, $x, OP_BITWISE_OR, 0, 0, NULL); }
    /* type1 */
    | regname[x] binop vatom '+' regname[y]
        { $$ = make_rhs(1, $x, $binop, $y, 1, $vatom); }
    | regname[x] binop vatom
        {   int t3op = $binop == OP_ADD || $binop == OP_SUBTRACT;
            int mult = ($binop == OP_SUBTRACT) ? -1 : 1;
            int op   = (mult < 0) ? OP_ADD : $binop;
            int type = (t3op && is_type3($vatom)) ? 3 : 1;
            $$ = make_rhs(type, $x, op, 0, mult, $vatom); }
    /* type2 */
    | vatom binop regname[x] '+' regname[y]
        { $$ = make_rhs(2, $x, $binop, $y, 1, $vatom); }
    | vatom binop regname[x]
        { $$ = make_rhs(2, $x, $binop, 0, 1, $vatom); }
    /* type3 */
    | vatom
        { $$ = make_rhs(is_type3($vatom) ? 3 : 0, 0, 0, 0, 1, $vatom); }
    /* syntax sugars */
    | unary_op regname[x] reloc_op vatom
        { $$ = make_unary($unary_op, $x,  0, $reloc_op, $vatom); }
    | unary_op regname[x] '+' regname[y]
        { $$ = make_unary($unary_op, $x, $y, 0, NULL); }
    | unary_op regname[x]
        { $$ = make_unary($unary_op, $x,  0, 0, NULL); }

regname
    : REGISTER { $regname = toupper($REGISTER) - 'A'; }

immediate
    : INTEGER
        {   $immediate.i = $INTEGER;
            $immediate.flags = 0; }
    | BITSTRING
        {   $immediate.i = $BITSTRING;
            $immediate.flags = IMM_IS_BITS; }
    | CHARACTER
        {   $immediate.i = $CHARACTER->tail[-1];
            $immediate.flags = IMM_IS_BITS;
            free_cstr($CHARACTER, 1); }

binop
    /* native ops */
    : '+'   { $$ = OP_ADD              ; }
    | '-'   { $$ = OP_SUBTRACT         ; }
    | '*'   { $$ = OP_MULTIPLY         ; }
    | '<'   { $$ = OP_COMPARE_LT       ; }
    | "=="  { $$ = OP_COMPARE_EQ       ; }
    | ">="  { $$ = OP_COMPARE_GE       ; }
    | '|'   { $$ = OP_BITWISE_OR       ; }
    | "|~"  { $$ = OP_BITWISE_ORN      ; }
    | '&'   { $$ = OP_BITWISE_AND      ; }
    | "&~"  { $$ = OP_BITWISE_ANDN     ; }
    | '^'   { $$ = OP_BITWISE_XOR      ; }
    | "<<"  { $$ = OP_SHIFT_LEFT       ; }
    | ">>"  { $$ = OP_SHIFT_RIGHT_ARITH; }
    | ">>>" { $$ = OP_SHIFT_RIGHT_LOGIC; }
    | "^^"  { $$ = OP_PACK             ; }
    | '@'   { $$ = OP_TEST_BIT         ; }
    /* ops implemented by syntax sugar */
    | '>'   { $$ = -OP_COMPARE_LT      ; }
    | "<="  { $$ = -OP_COMPARE_GE      ; }

unary_op
    : '~'   { $$ = OP_BITWISE_ORN; }
    | '-'   { $$ = OP_SUBTRACT; }

arrow
    : "<-"  { $$ = 0; }
    | "->"  { $$ = 1; }

reloc_op
    : '+'   { $$ = +1; }
    | '-'   { $$ = -1; }

vexpr : expr        { $$ = $1; validate_expr(pd, $1, 0); }
vatom : atom        { $$ = $1; validate_expr(pd, $1, 0); }

expr
    : atom          { $$ = $1; ce_eval_const(pd, $1, &$1->i); }
    | binop_expr    { $$ = $1; ce_eval_const(pd, $1, &$1->i); }

eref : '@' SYMBOL { $$ = make_ref(pd, &yylloc, CE_EXT, $SYMBOL); free_cstr($SYMBOL, 1); }

binop_expr
    : expr[x]  '+'  expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  '+', $x, $y, 0); }
    | expr[x]  '-'  expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  '-', $x, $y, RHS_NEGATE); }
    | expr[x]  '*'  expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  '*', $x, $y, FORBID_LHS); }
    | expr[x]  '/'  expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  '/', $x, $y, FORBID_LHS); }
    | expr[x]  '^'  expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  '^', $x, $y, FORBID_LHS); }
    | expr[x]  '&'  expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  '&', $x, $y, SPECIAL_LHS); }
    | expr[x]  '|'  expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  '|', $x, $y, FORBID_LHS); }
    | expr[x]  "<<" expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  LSH, $x, $y, FORBID_LHS); }
    | expr[x]  ">>" expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2, RSHA, $x, $y, SPECIAL_LHS); }
    | expr[x] ">>>" expr[y] { $$ = make_expr(pd, &yylloc, CE_OP2,  RSH, $x, $y, FORBID_LHS); }

const_unary_op
    : '~'   { $$ = '~'; }
    | '-'   { $$ = '-'; }

atom
    : eref
    | '(' expr ')'
        {   $$ = $expr; }
    | const_unary_op atom[inner]
        {   $$ = make_expr(pd, &yylloc, CE_OP1, $const_unary_op, $inner, NULL, 0);
            ce_eval_const(pd, $$, &$$->i); }
    | immediate
        {   $$ = make_expr(pd, &yylloc, CE_IMM, 0, NULL, NULL, $immediate.flags);
            $$->i = $immediate.i; }
    | '.'
        {   $$ = make_expr(pd, &yylloc, CE_ICI, 0, NULL, NULL, IMM_IS_BITS | IS_DEFERRED); }
    | LOCAL
        {   $$ = make_ref(pd, &yylloc, CE_SYM, $LOCAL); }

%%

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *fmt, ...)
{
    va_list vl;
    int col = locp->first_column + 1;

    fflush(stderr);
    if (col <= 1)
        fprintf(stderr, "%s\n", pd->lexstate.savep[!pd->lexstate.swap]);
    fprintf(stderr, "%s\n", pd->lexstate.savep[pd->lexstate.swap]);
    fprintf(stderr, "%*s", col, "^");
    int len = locp->last_column - locp->first_column;
    while (len-- > 0)
        fputc('^', stderr);
    fputc('\n', stderr);

    va_start(vl, fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);

    fprintf(stderr, " at line %d column %d at `%s'\n",
            locp->first_line, col, tenyr_get_text(pd->scanner));

    pd->errored++;
    return 0;
}

static struct element *make_insn_general(struct parse_data *pd,
        struct expr *lhs, int arrow, struct expr *expr)
{
    struct element *elem = calloc(1, sizeof *elem);
    int dd = ((lhs->deref | (!arrow && expr->deref)) << 1) | expr->deref;

    elem->insn.u.typeany.p  = expr->type;
    elem->insn.u.typeany.dd = dd;
    elem->insn.u.typeany.z  = lhs->x;
    elem->insn.u.typeany.x  = expr->x;

    free(lhs);

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

    free(expr);

    return elem;
}

static struct const_expr *add_deferred_expr(struct parse_data *pd,
        struct const_expr *ce, int mult, uint32_t *dest, int width, int flags)
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

static struct const_expr *make_expr(struct parse_data *pd, YYLTYPE *locp,
        enum const_expr_type type, int op, struct const_expr *left,
        struct const_expr *right, int flags)
{
    struct const_expr *n = calloc(1, sizeof *n);

    if (left && (left->flags & IS_EXTERNAL) && (flags & FORBID_LHS))
        tenyr_error(locp, pd, "Expression contains an invalid use of a "
                              "deferred expression");

    n->type  = type;
    n->op    = op;
    n->left  = left;
    n->right = right;
    n->insn  = NULL;    // top expr will have its insn filled in
    n->flags = (left  ? left->flags  : 0) |
               (right ? right->flags : 0) | flags;
    n->srcloc = *locp;

    return n;
}

static struct expr *make_rhs(int type, int x, int op, int y, int mult,
        struct const_expr *defexpr)
{
    struct expr *e = calloc(1, sizeof *e);

    e->type  = type;
    e->deref = 0;
    e->x     = x;
    e->op    = op;
    e->y     = y;
    e->mult  = mult;
    e->ce    = defexpr;

    if (op < 0) { // syntax sugar, swapping operands
        assert(type != 3);
        e->op = -op;
        switch (type) {
            case 0: e->x = y; e->y = x; break;
            case 1: e->type = 2; break;
            case 2: e->type = 1; break;
        }
    }

    if (defexpr) {
        if (op == OP_PACK)
            e->ce->flags |= IGNORE_WIDTH;
        e->i = 0xfffffbad; // put in a placeholder that must be overwritten
    } else
        e->i = 0; // there was no expr ; zero defined by language

    return e;
}

static struct expr *make_unary(int op, int x, int y, int mult,
        struct const_expr *defexpr)
{
    // tenyr has no true unary ops, but the following sugars are recognised by
    // the assembler and converted into their corresponding binary equivalents :
    //
    // b <- ~b + 2  becomes     b <- a |~ b + 2 (type 0)
    // b <- ~b + c  becomes     b <- 0 |~ b + c (type 2)
    // b <- -b + 2  becomes     b <- a -  b + 2 (type 0)
    // b <- -b + c  becomes     b <- 0 -  b + c (type 2)

    int type = defexpr ? 0 : 2;
    int xx   = defexpr ? 0 : x;
    int yy   = defexpr ? x : y;
    return make_rhs(type, xx, op, yy, mult, defexpr);
}

static void free_cstr(struct cstr *cs, int recurse)
{
    if (!cs)
        return;

    if (recurse)
        free_cstr(cs->right, recurse);

    free(cs->head);
    free(cs);
}

static struct element_list *make_utf32(struct cstr *cs)
{
    struct element_list *result = NULL, **rp = &result;

    struct cstr *p = cs;
    struct element_list *t = *rp;

    while (p) {
        char *h = p->head;
        for (; h < p->tail; h++) {
            *rp = calloc(1, sizeof **rp);
            result->tail = t = *rp;
            rp = &t->next;
            t->elem = calloc(1, sizeof *t->elem);

            t->elem->insn.u.word = *h;
            t->elem->insn.size = 1;
        }

        p = p->right;
    }

    return result;
}

static int add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n)
{
    struct symbol_list *l = calloc(1, sizeof *l);

    l->next = pd->symbols;
    l->symbol = n;
    pd->symbols = l;

    return 0;
}

static char *coalesce_string(const struct cstr *s)
{
    const struct cstr *p = s;
    size_t len = 0;
    while (p) {
        len += p->tail - p->head;
        p = p->right;
    }

    char *ret = malloc(len + 1);
    p = s;
    len = 0;
    while (p) {
        ptrdiff_t size = p->tail - p->head;
        memcpy(&ret[len], p->head, size);
        len += size;
        p = p->right;
    }

    ret[len] = '\0';

    return ret;
}

static int add_symbol_to_insn(struct parse_data *pd, YYLTYPE *locp,
        struct element *insn, struct cstr *symbol)
{
    struct symbol *n = calloc(1, sizeof *n);
    n->column   = locp->first_column;
    n->lineno   = locp->first_line;
    n->resolved = 0;
    n->next     = insn->symbol;
    n->unique   = 1;
    n->name     = coalesce_string(symbol);
    insn->symbol = n;

    return add_symbol(locp, pd, n);
}

static struct element_list *make_data(struct parse_data *pd,
        struct const_expr_list *list)
{
    struct element_list *result = NULL, **rp = &result;

    struct const_expr_list *p = list;
    while (p) {
        *rp = calloc(1, sizeof **rp);
        struct element_list *q = *rp;
        result->tail = *rp;
        rp = &q->next;

        struct element *e = q->elem = calloc(1, sizeof *q->elem);
        e->insn.size = 1;
        add_deferred_expr(pd, p->ce, 1, &e->insn.u.word, WORD_BITWIDTH, 0);
        p->ce->insn = e;
        struct const_expr_list *temp = p;
        p = p->right;
        free(temp);
    }

    return result;
}

static struct element_list *make_zeros(struct parse_data *pd, YYLTYPE *locp,
        struct const_expr *size)
{
    if (size->flags & IS_DEFERRED)
        tenyr_error(locp, pd, "size expression for .zero must not "
                              "depend on symbol values");

    // Continue even in the error case so we return a real data element so
    // further errors can be collected and reported. Returning NULL here
    // without adding additional complexity to data-users in the grammar would
    // cause a segfault.

    struct element_list *result = calloc(1, sizeof *result);
    result->elem = calloc(1, sizeof *result->elem);
    result->tail = result;
    ce_eval_const(pd, size, &result->elem->insn.size);
    return result;
}

static struct directive *make_global(struct parse_data *pd, YYLTYPE *locp,
        const struct cstr *symbol)
{
    struct directive *result = calloc(1, sizeof *result);
    result->type = D_GLOBAL;
    struct global_list *g = result->data = malloc(sizeof *g);
    strcopy(g->name, symbol->head, symbol->tail - symbol->head + 1);
    return result;
}

static struct directive *make_set(struct parse_data *pd, YYLTYPE *locp,
        const struct cstr *symbol, struct const_expr *expr)
{
    struct directive *result = calloc(1, sizeof *result);
    result->type = D_SET;

    struct symbol *n = result->data = calloc(1, sizeof *n);
    n->column   = locp->first_column;
    n->lineno   = locp->first_line;
    n->resolved = 0;
    n->next     = NULL;
    n->ce       = expr;
    n->unique   = 0;
    // XXX validate symbol length
    n->name = coalesce_string(symbol);

    add_symbol(locp, pd, n);

    return result;
}

static void handle_directive(struct parse_data *pd, YYLTYPE *locp,
        struct directive *d, struct element_list **context)
{
    switch (d->type) {
        case D_GLOBAL: {
            struct global_list *g = d->data;
            g->next = pd->globals;
            pd->globals = g;
            break;
        }
        case D_SET: {
            struct symbol *sym = d->data;
            sym->ce->deferred = context;
            break;
        }
        default:
            tenyr_error(locp, pd, "Unknown directive type %d in %s",
                        d->type, __func__);
    }

    free(d);
}

static int is_type3(struct const_expr *ce)
{
    int is_bits  = ce->flags & IMM_IS_BITS;
    int deferred = ce->flags & IS_DEFERRED;
    int32_t extended = SEXTEND32(SMALL_IMMEDIATE_BITWIDTH,ce->i);
    /* Large immediates and ones that should be expressed in
     * hexadecimal use type3. */
    if (is_bits || deferred || ce->i != extended)
        return 1;

    return 0;
}

static struct const_expr *make_ref(struct parse_data *pd, YYLTYPE *locp,
        int type, const struct cstr *symbol)
{
    int flags = IMM_IS_BITS | IS_DEFERRED;
    if (type == CE_EXT)
        flags |= IS_EXTERNAL;
    struct const_expr *eref = make_expr(pd, locp, CE_EXT, 0, NULL, NULL, flags);
    struct symbol *s;
    if ((s = symbol_find(pd->symbols, symbol->head))) {
        eref->symbol = s;
    } else {
        strcopy(eref->symbolname, symbol->head, sizeof eref->symbolname);
    }

    return eref;
}

static struct const_expr_list *make_expr_list(struct const_expr *expr,
        struct const_expr_list *right)
{
    struct const_expr_list *result = calloc(1, sizeof *result);
    result->right = right;
    result->ce = expr;
    return result;
}

static int validate_expr(struct parse_data *pd, struct const_expr *e, int level)
{
    int ok = 1;

    if (!e)
        return 0;

    ok &= validate_expr(pd, e->left , level + 1);
    ok &= validate_expr(pd, e->right, level + 1);

    // check both left and right to make sure we validate only binary ops
    if (    e->left && e->right &&
            (e->left->flags & IS_EXTERNAL) &&
            (e->flags & SPECIAL_LHS))
    {
        // Special rules apply to right-shifts of 12 and masks of 0xfff. This
        // is how we express breaking down a large offset into a 20-bit chunk
        // and a 12-bit chunk that we reassemble using OP_PACK.
        // TODO implement the special relocation types in the linker
        // XXX how to characterise the legal possibilities ?
        // B <- ((@xx - .) >> 12) ; C <- B ^^ ((@xx - .) & 0xfff)
        // B <- ( @xx      >> 12) ; C <- B ^^ ( @xx      & 0xfff)
        switch (e->op) {
            case RSHA:  ok &= (level == 0); ok &= e->right->i == 12;    break;
            case '&':   ok &= (level == 0); ok &= e->right->i == 0xfff; break;
            default:    ok &= 0; break;
        }

        if (!ok)
            // XXX at this point e->srcloc may not correspond to lexstate
            // TODO make an opaque "where this was" object
            tenyr_error(&e->srcloc, pd,
                        "Expression contains an invalid use of a "
                        "deferred expression");
    }

    return ok;
}

