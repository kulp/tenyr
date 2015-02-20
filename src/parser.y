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
static struct element_list *make_ascii(struct cstr *cs);
static struct element_list *make_utf32(struct cstr *cs);
static struct symbol *add_symbol_to_insn(YYLTYPE *locp, struct element *insn,
        const char *symbol);
static int add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n);
static int check_add_symbol(YYLTYPE *locp, struct parse_data *pd, struct symbol *n);
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
}
%parse-param { struct parse_data *pd }
%name-prefix "tenyr_"

%start top

/* precedence rules only matter in constant expressions */
%left '|'
%left '^'
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

/* synonyms for literal string tokens */
%token LSH  "<<"
%token RSH  ">>>"
%token RSHA ">>"
%token EQ   "=="
%token GE   ">="
%token LE   "<="
%token ORN  "|~"
%token ANDN "&~"
%token PACK "^^"

%token WORD     ".word"
%token ASCII    ".ascii"
%token UTF32    ".utf32"
%token GLOBAL   ".global"
%token SET      ".set"
%token ZERO     ".zero"

%type <ce> const_expr const_binop_expr reloc_expr reloc_atom const_atom eref
%type <cl> reloc_expr_list
%type <cstr> string
%type <dctv> directive
%type <expr> rhs_plain rhs_sugared rhs_deref lhs_plain lhs_deref
%type <i> arrow regname reloc_op
%type <imm> immediate
%type <insn> insn insn_inner
%type <op> native_op sugar_op sugar_unary_op const_unary_op
%type <program> program program_elt ascii utf32 data string_or_data
%type <str> symbol symbol_list

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
    : /* empty */
        {   pd->top = NULL; pd->top->tail = pd->top; }
    | program seps
        {   if (pd->top != NULL) {
                // Allocate a phantom entry to provide a final context element
                pd->top->tail->next = calloc(1, sizeof *pd->top->tail);
                pd->top->tail = pd->top->tail->next;
            }
        }

string_or_data
    : ascii
    | utf32
    | data
    | symbol ':' opt_nl string_or_data[inner]
        {   $$ = $inner;
            struct symbol *n = add_symbol_to_insn(&yyloc, $inner->elem,
                    $symbol.buf);
            if (check_add_symbol(&yyloc, pd, n))
                YYABORT;
        }

opt_nl
    : '\n' opt_nl
    | /* empty */

program
    :   /* empty */
        {   $$ = NULL; }
    | program_elt
        {   pd->top = $$ = $1; }
    | program seps program_elt
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
    | program seps directive
        {   handle_directive(pd, &yylloc, $directive, pd->top ? &pd->top->tail->next : &pd->top); }
    | directive
        {   assert(pd->top == NULL);
            pd->top = $$ = NULL;
            handle_directive(pd, &yylloc, $directive, &pd->top); }

program_elt
    : string_or_data
    | insn
        {   $program_elt = calloc(1, sizeof *$program_elt);
            $program_elt->elem = $insn;
            $program_elt->tail = $program_elt; }

sep
    : '\n'
    | ';'

seps
    : seps sep
    | sep

insn
    : ILLEGAL
        {   $$ = calloc(1, sizeof *$$);
            $$->insn.size = 1;
            $$->insn.u.word = 0xcfffffff; /* P <- -1 */ }
    | insn_inner
    | symbol ':' opt_nl insn[inner]
        {   $$ = $inner;
            struct symbol *n = add_symbol_to_insn(&yyloc, $inner, $symbol.buf);
            if (check_add_symbol(&yyloc, pd, n))
                YYABORT;
        }

symbol
    : SYMBOL
    | LOCAL

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

string
    : STRING
        {   $$ = calloc(1, sizeof *$$);
            $$->len = $STRING.len;
            $$->str = malloc($$->len + 1);
            strcopy($$->str, $STRING.buf, $$->len + 1); }
    | STRING string[inner]
        {   $$ = calloc(1, sizeof *$$);
            $$->len = $STRING.len;
            $$->str = malloc($$->len + 1);
            strcopy($$->str, $STRING.buf, $$->len + 1);
            $$->right = $inner; }

utf32
    : ".utf32" string
        {   tenyr_pop_state(pd->scanner); $utf32 = make_utf32($string); }

ascii
    : ".ascii" string
        {   tenyr_pop_state(pd->scanner); $ascii = make_ascii($string); }

data
    : ".word" opt_nl reloc_expr_list
        {   tenyr_pop_state(pd->scanner); $data = make_data(pd, $reloc_expr_list); }
    | ".zero" opt_nl const_expr
        {   tenyr_pop_state(pd->scanner); $data = make_zeros(pd, &yylloc, $const_expr); }

directive
    : ".global" opt_nl symbol_list
        {   tenyr_pop_state(pd->scanner); $directive = make_global(pd, &yylloc, &$symbol_list); }
    | ".set" opt_nl SYMBOL ',' reloc_expr
        {   tenyr_pop_state(pd->scanner); $directive = make_set(pd, &yylloc, &$SYMBOL, $reloc_expr); }

symbol_list
    : SYMBOL /* TODO permit comma-separated symbol lists for GLOBAL */

reloc_expr_list
    : reloc_expr[expr]
        {   $$ = calloc(1, sizeof *$$);
            $$->right = NULL;
            $$->ce = $expr; }
    | reloc_expr[expr] ',' opt_nl reloc_expr_list[inner]
        {   $$ = calloc(1, sizeof *$$);
            $$->right = $inner;
            $$->ce = $expr; }

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
    | regname[x] native_op regname[y] reloc_op reloc_expr
        { $rhs_plain = make_expr(0, $x, $native_op, $y, $reloc_op == '+' ? 1 : -1, $reloc_expr); }
    | regname[x] native_op regname[y]
        { $rhs_plain = make_expr(0, $x, $native_op, $y, 0, NULL); }
    | regname[x]
        { $rhs_plain = make_expr(0, $x, OP_BITWISE_OR, 0, 0, NULL); }
    | regname[x] sugar_op regname[y] '+' reloc_expr
        { $rhs_plain = make_expr(0, $x, -$sugar_op, $y, 1, $reloc_expr); }
    | regname[x] sugar_op regname[y]
        { $rhs_plain = make_expr(0, $x, -$sugar_op, $y, 0, NULL); }
    /* type1 */
    | regname[x] native_op reloc_expr '+' regname[y]
        { $rhs_plain = make_expr(1, $x, $native_op, $y, 1, $reloc_expr); }
    | regname[x] native_op reloc_expr
        { $rhs_plain = make_expr(1, $x, $native_op, 0, 1, $reloc_expr); }
    | reloc_expr sugar_op regname[x] '+' regname[y]
        { $rhs_plain = make_expr(1, $x, -$sugar_op, $y, 1, $reloc_expr); }
    | reloc_expr sugar_op regname[x]
        { $rhs_plain = make_expr(1, $x, -$sugar_op, 0, 1, $reloc_expr); }
    /* type2 */
    | reloc_expr native_op regname[x] '+' regname[y]
        { $rhs_plain = make_expr(2, $x, $native_op, $y, 1, $reloc_expr); }
    | reloc_expr native_op regname[x]
        { $rhs_plain = make_expr(2, $x, $native_op, 0, 1, $reloc_expr); }
    | regname[x] sugar_op reloc_expr '+' regname[y]
        { $rhs_plain = make_expr(2, $x, -$sugar_op, $y, 1, $reloc_expr); }
    | regname[x] sugar_op reloc_expr
        { $rhs_plain = make_expr(2, $x, -$sugar_op, 0, 1, $reloc_expr); }
    /* type3 */
    | reloc_expr
        {   struct const_expr *ce = $reloc_expr;
            int is_bits  = ce->flags & IMM_IS_BITS;
            int deferred = ce->flags & IS_DEFERRED;
            /* Large immediates and ones that should be expressed in
             * hexadecimal use type3 ; others use type0. */
            int type = 0;
            if (is_bits || deferred || ce->i != SEXTEND32(SMALL_IMMEDIATE_BITWIDTH,ce->i))
                type = 3;
            $rhs_plain = make_expr(type, 0, OP_BITWISE_OR, 0, 1, ce);
        }

rhs_sugared
    : sugar_unary_op regname[x]
        { $rhs_sugared = make_unary($sugar_unary_op, $x,  0, 0, NULL); }
    | sugar_unary_op regname[x] '+' regname[y]
        { $rhs_sugared = make_unary($sugar_unary_op, $x, $y, 0, NULL); }
    | sugar_unary_op regname[x] reloc_op reloc_expr
        { $rhs_sugared = make_unary($sugar_unary_op, $x,  0, $reloc_op == '+' ? 1 : -1, $reloc_expr); }

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
    | '|'   { $native_op = OP_BITWISE_OR       ; }
    | "|~"  { $native_op = OP_BITWISE_ORN      ; }
    | '&'   { $native_op = OP_BITWISE_AND      ; }
    | "&~"  { $native_op = OP_BITWISE_ANDN     ; }
    | '^'   { $native_op = OP_BITWISE_XOR      ; }
    | "<<"  { $native_op = OP_SHIFT_LEFT       ; }
    | ">>"  { $native_op = OP_SHIFT_RIGHT_ARITH; }
    | ">>>" { $native_op = OP_SHIFT_RIGHT_LOGIC; }
    | "^^"  { $native_op = OP_PACK             ; }
    | '@'   { $native_op = OP_TEST_BIT         ; }

sugar_op
    : '>'   { $sugar_op = -OP_COMPARE_LT; }
    | "<="  { $sugar_op = -OP_COMPARE_GE; }

sugar_unary_op
    : '~' { $sugar_unary_op = OP_BITWISE_ORN; }
    | '-' { $sugar_unary_op = OP_SUBTRACT; }

arrow
    : tol { $arrow = 0; }
    | tor { $arrow = 1; }

tol : TOL { tenyr_pop_state(pd->scanner); }
tor : TOR { tenyr_pop_state(pd->scanner); }

reloc_op
    : '+' { $reloc_op = '+'; }
    | '-' { $reloc_op = '-'; }

reloc_expr
    : const_atom
    | reloc_atom

reloc_atom
    : eref
    | '(' reloc_atom[inner] ')'
        {   $$ = $inner; }
    | '(' eref reloc_op const_expr ')'
        {   $$ = make_const_expr(CE_OP2, $reloc_op, $eref, $const_expr, 0); }

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
        {   $$ = make_const_expr(CE_IMM, 0, NULL, NULL, $immediate.is_bits ? IMM_IS_BITS : 0);
            $$->i = $immediate.i; }
    | '.'
        {   $$ = make_const_expr(CE_ICI, 0, NULL, NULL, IMM_IS_BITS | IS_DEFERRED); }
    | LOCAL
        {   $$ = make_const_expr(CE_SYM, 0, NULL, NULL, IMM_IS_BITS | IS_DEFERRED);
            struct symbol *s;
            if ((s = symbol_find(pd->symbols, $LOCAL.buf))) {
                $$->symbol = s;
            } else {
                strcopy($$->symbolname, $LOCAL.buf, sizeof $$->symbolname);
            }
        }

eref
    : '@' SYMBOL
        {   $eref = make_const_expr(CE_EXT, 0, NULL, NULL, IMM_IS_BITS | IS_DEFERRED);
            struct symbol *s;
            if ((s = symbol_find(pd->symbols, $SYMBOL.buf))) {
                $eref->symbol = s;
            } else {
                strcopy($eref->symbolname, $SYMBOL.buf, sizeof $eref->symbolname);
            }
        }

%%

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *fmt, ...)
{
    va_list vl;
    int col = locp->first_column + 1;

    fflush(stderr);
    if (col <= 1)
        fprintf(stderr, "%s\n", pd->lexstate.savep[!pd->lexstate.swap]);
    fprintf(stderr, "%s\n", pd->lexstate.savep[pd->lexstate.swap]);
    fprintf(stderr, "%*s\n", col, "^");

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
    unsigned wpos = 0; // position in the word
    struct element_list *t = *rp;

    while (p) {
        unsigned spos = 0; // position in the string
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
                result->tail = t = *rp;
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

    l->next = pd->symbols;
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
        tenyr_error(locp, pd, "Error adding symbol '%s' (already exists ?)", n->name);
        return 1;
    } else
    #endif
        return add_symbol(locp, pd, n);
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

