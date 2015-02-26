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
#include "asm.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *fmt, ...);
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
static struct element_list *make_utf32(struct cstr *cs);
static int add_symbol_to_insn(struct parse_data *pd, YYLTYPE *locp,
        struct element *insn, const char *symbol);
static int add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n);
static struct element_list *make_data(struct parse_data *pd, struct
        const_expr_list *list);
static struct element_list *make_zeros(struct parse_data *pd, YYLTYPE *locp,
        struct const_expr *size);
static struct directive *make_global(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *sym);
static struct directive *make_set(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *sym, struct const_expr *expr);
static void handle_directive(struct parse_data *pd, YYLTYPE *locp, struct
        directive *d, struct element_list **context);
static int is_type3(struct const_expr *ce);
static struct const_expr *make_ref(struct parse_data *pd, int type,
        struct strbuf *symbol);
static struct const_expr_list *make_reloc_list(struct const_expr *expr,
        struct const_expr_list *right);
static struct cstr *make_string(const struct strbuf *str, struct cstr *right);

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
%token <str>    SYMBOL LOCAL STRING CHARACTER
%token          ILLEGAL

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

%type <ce>      const_expr const_binop_expr reloc_expr const_atom eref
%type <cl>      reloc_list
%type <cstr>    string
%type <dctv>    directive
%type <expr>    rhs rhs_plain lhs lhs_plain
%type <i>       arrow regname reloc_op
%type <imm>     immediate
%type <op>      binop unary_op const_unary_op
%type <program> program program_elt data insn
%type <str>     symbol

%union {
    int                     op; ///< negative ops mark swapped operands
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
    struct strbuf {
        int pos, len;
        char buf[LINE_LEN];
    } str;
}

%%

top
    : opt_terminators
        {   pd->top = NULL; }
    | opt_terminators program
        {   // Allocate a phantom entry to provide a final context element
            *(pd->top ? &pd->top->tail->next : &pd->top) = calloc(1, sizeof *pd->top); }

opt_nl
    : opt_nl '\n'
    | /* empty */

program
    : program_elt
        {   pd->top = $$ = $1; pd->top->tail = pd->top; }
    | program program_elt
        {   struct element_list *p = $1, *d = $program_elt;
            if (p == NULL) {
                p = d;
            } else if (d != NULL) { // an empty string is NULL
                d->prev = p->tail;
                p->tail->next = d;
                p->tail = d->tail;
            }
            pd->top = $$ = p;
        }
    | program directive
        {   handle_directive(pd, &yylloc, $directive, pd->top ? &pd->top->tail->next : &pd->top); }
    | directive
        {   pd->top = $$ = NULL;
            handle_directive(pd, &yylloc, $directive, &pd->top); }

program_elt
    : data terminators
    | insn terminators
    | symbol ':' opt_nl program_elt[inner]
        {   add_symbol_to_insn(pd, &yyloc, ($$ = $inner)->elem, $symbol.buf); }

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
    : STRING                {   $$ = make_string(&$STRING, NULL);   }
    | STRING string[right]  {   $$ = make_string(&$STRING, $right); }

data
    : ".word" opt_nl reloc_list {   POP; $$ = make_data(pd, $reloc_list);           }
    | ".zero" opt_nl const_expr {   POP; $$ = make_zeros(pd, &yylloc, $const_expr); }
    | ".utf32" string           {   POP; $$ = make_utf32($string);                  }

directive
    : ".global" opt_nl SYMBOL terminators
        {   POP; $directive = make_global(pd, &yylloc, &$SYMBOL); }
    | ".set" opt_nl SYMBOL ',' reloc_expr terminators
        {   POP; $directive = make_set(pd, &yylloc, &$SYMBOL, $reloc_expr); }

reloc_list
    : reloc_expr
        {   $$ = make_reloc_list($reloc_expr, NULL); }
    | reloc_expr ',' opt_nl reloc_list[right]
        {   $$ = make_reloc_list($reloc_expr, $right); }

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
    : regname[x] binop regname[y] reloc_op reloc_expr
        { $$ = make_expr(0, $x, $binop, $y, $reloc_op, $reloc_expr); }
    | regname[x] binop regname[y]
        { $$ = make_expr(0, $x, $binop, $y, 0, NULL); }
    | regname[x]
        { $$ = make_expr(0, $x, OP_BITWISE_OR, 0, 0, NULL); }
    /* type1 */
    | regname[x] binop reloc_expr '+' regname[y]
        { $$ = make_expr(1, $x, $binop, $y, 1, $reloc_expr); }
    | regname[x] binop reloc_expr
        {   int t3op = $binop == OP_ADD || $binop == OP_SUBTRACT;
            int mult = ($binop == OP_SUBTRACT) ? -1 : 1;
            int op   = (mult < 0) ? OP_ADD : $binop;
            int type = (t3op && is_type3($reloc_expr)) ? 3 : 1;
            $$ = make_expr(type, $x, op, 0, mult, $reloc_expr); }
    /* type2 */
    | reloc_expr binop regname[x] '+' regname[y]
        { $$ = make_expr(2, $x, $binop, $y, 1, $reloc_expr); }
    | reloc_expr binop regname[x]
        { $$ = make_expr(2, $x, $binop, 0, 1, $reloc_expr); }
    /* type3 */
    | reloc_expr
        { $$ = make_expr(is_type3($reloc_expr) ? 3 : 0, 0, 0, 0, 1, $reloc_expr); }
    /* syntax sugars */
    | unary_op regname[x] reloc_op reloc_expr
        { $$ = make_unary($unary_op, $x,  0, $reloc_op, $reloc_expr); }
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
        {   $immediate.i = $CHARACTER.buf[$CHARACTER.pos - 1];
            $immediate.flags = IMM_IS_BITS; }

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
    : '~' { $$ = OP_BITWISE_ORN; }
    | '-' { $$ = OP_SUBTRACT; }

arrow
    : "<-" { $arrow = 0; }
    | "->" { $arrow = 1; }

reloc_op
    : '+' { $reloc_op = +1; }
    | '-' { $reloc_op = -1; }

reloc_expr
    : eref
    | const_atom
    | '(' eref reloc_op const_atom ')'
        {   $$ = make_const_expr(CE_OP2, "- +"[$reloc_op + 1], $eref, $const_atom, 0); }

eref : '@' SYMBOL { $$ = make_ref(pd, CE_EXT, &$SYMBOL); }

const_expr
    : const_atom
    | const_binop_expr { ce_eval(pd, NULL, NULL, $$, 0, NULL, NULL, &$$->i); }

const_binop_expr
    : const_expr[x]  '+'  const_expr[y] { $$ = make_const_expr(CE_OP2,  '+', $x, $y, 0); }
    | const_expr[x]  '-'  const_expr[y] { $$ = make_const_expr(CE_OP2,  '-', $x, $y, 0); }
    | const_expr[x]  '*'  const_expr[y] { $$ = make_const_expr(CE_OP2,  '*', $x, $y, 0); }
    | const_expr[x]  '/'  const_expr[y] { $$ = make_const_expr(CE_OP2,  '/', $x, $y, 0); }
    | const_expr[x]  '^'  const_expr[y] { $$ = make_const_expr(CE_OP2,  '^', $x, $y, 0); }
    | const_expr[x]  '&'  const_expr[y] { $$ = make_const_expr(CE_OP2,  '&', $x, $y, 0); }
    | const_expr[x]  "<<" const_expr[y] { $$ = make_const_expr(CE_OP2,  LSH, $x, $y, 0); }
    | const_expr[x]  ">>" const_expr[y] { $$ = make_const_expr(CE_OP2, RSHA, $x, $y, 0); }
    | const_expr[x] ">>>" const_expr[y] { $$ = make_const_expr(CE_OP2,  RSH, $x, $y, 0); }

const_unary_op
    : '~' { $const_unary_op = '~'; }
    | '-' { $const_unary_op = '-'; }

const_atom
    : const_unary_op const_atom[inner]
        {   $$ = make_const_expr(CE_OP1, $const_unary_op, $inner, NULL, 0);
            ce_eval(pd, NULL, NULL, $$, 0, NULL, NULL, &$$->i); }
    | '(' const_expr ')'
        {   $$ = $const_expr; }
    | immediate
        {   $$ = make_const_expr(CE_IMM, 0, NULL, NULL, $immediate.flags);
            $$->i = $immediate.i; }
    | '.'
        {   $$ = make_const_expr(CE_ICI, 0, NULL, NULL, IMM_IS_BITS | IS_DEFERRED); }
    | LOCAL
        {   $$ = make_ref(pd, CE_SYM, &$LOCAL); }

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

static struct element *make_insn_general(struct parse_data *pd, struct
        expr *lhs, int arrow, struct expr *expr)
{
    struct element *elem = calloc(1, sizeof *elem);

    elem->insn.u.typeany.p  = expr->type;
    elem->insn.u.typeany.dd = ((lhs->deref | (!arrow && expr->deref)) << 1) | expr->deref;
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
    int wpos = 0; // position in the word
    struct element_list *t = *rp;

    while (p) {
        int spos = 0; // position in the string
        int len = p->len;
        for (; len > 0; wpos++, spos++, len--) {
            *rp = calloc(1, sizeof **rp);
            (*rp)->prev = t;
            result->tail = t = *rp;
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

static int add_symbol_to_insn(struct parse_data *pd, YYLTYPE *locp,
        struct element *insn, const char *symbol)
{
    struct symbol *n = calloc(1, sizeof *n);
    n->column   = locp->first_column;
    n->lineno   = locp->first_line;
    n->resolved = 0;
    n->next     = insn->symbol;
    n->unique   = 1;
    strcopy(n->name, symbol, sizeof n->name);
    insn->symbol = n;

    return add_symbol(locp, pd, n);
}

static int add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n)
{
    struct symbol_list *l = calloc(1, sizeof *l);

    l->next = pd->symbols;
    l->symbol = n;
    pd->symbols = l;

    return 0;
}

static struct element_list *make_data(struct parse_data *pd, struct const_expr_list *list)
{
    struct element_list *result = NULL, **rp = &result;

    struct const_expr_list *p = list;
    while (p) {
        *rp = calloc(1, sizeof **rp);
        struct element_list *q = *rp;
        q->prev = result->tail;
        result->tail = *rp;
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

static struct element_list *make_zeros(struct parse_data *pd, YYLTYPE *locp, struct const_expr *size)
{
    if (size->flags & IS_DEFERRED) {
        tenyr_error(locp, pd, "size expression for .zero must not depend on symbol values");
        return NULL;
    }

    struct element_list *result = calloc(1, sizeof *result);
    result->elem = calloc(1, sizeof *result->elem);
    result->tail = result;
    ce_eval(pd, NULL, NULL, size, 0, NULL, NULL, &result->elem->insn.size);
    return result;
}

static struct directive *make_global(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *symbol)
{
    struct directive *result = calloc(1, sizeof *result);
    result->type = D_GLOBAL;
    struct global_list *g = result->data = malloc(sizeof *g);
    strcopy(g->name, symbol->buf, symbol->len + 1);
    return result;
}

static struct directive *make_set(struct parse_data *pd, YYLTYPE *locp,
        const struct strbuf *symbol, struct const_expr *expr)
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
    // symbol length has already been validated by grammar
    strcopy(n->name, symbol->buf, symbol->len + 1);

    add_symbol(locp, pd, n);

    return result;
}

static void handle_directive(struct parse_data *pd, YYLTYPE *locp, struct
        directive *d, struct element_list **context)
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
            tenyr_error(locp, pd, "Unknown directive type %d in %s", d->type, __func__);
    }

    free(d);
}

static int is_type3(struct const_expr *ce)
{
    int is_bits  = ce->flags & IMM_IS_BITS;
    int deferred = ce->flags & IS_DEFERRED;
    /* Large immediates and ones that should be expressed in
     * hexadecimal use type3. */
    if (is_bits || deferred || ce->i != SEXTEND32(SMALL_IMMEDIATE_BITWIDTH,ce->i))
        return 1;

    return 0;
}

static struct const_expr *make_ref(struct parse_data *pd, int type, struct strbuf *symbol)
{
    struct const_expr *eref = make_const_expr(CE_EXT, 0, NULL, NULL, IMM_IS_BITS | IS_DEFERRED);
    struct symbol *s;
    if ((s = symbol_find(pd->symbols, symbol->buf))) {
        eref->symbol = s;
    } else {
        strcopy(eref->symbolname, symbol->buf, sizeof eref->symbolname);
    }

    return eref;
}

static struct const_expr_list *make_reloc_list(struct const_expr *expr, struct const_expr_list *right)
{
    struct const_expr_list *result = calloc(1, sizeof *result);
    result->right = right;
    result->ce = expr;
    return result;
}

static struct cstr *make_string(const struct strbuf *str, struct cstr *right)
{
    struct cstr *result = calloc(1, sizeof *result);
    result->right = right;
    result->len = str->len;
    result->str = malloc(str->len + 1);
    strcopy(result->str, str->buf, result->len + 1);
    return result;
}

