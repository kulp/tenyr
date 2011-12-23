%{
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "parser_global.h"
#include "lexer.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s);
static struct const_expr *add_relocation(struct parse_data *pd, struct
        const_expr *ce, int mult, uint32_t *dest, int width);
static struct const_expr *make_const_expr(int type, int op, struct const_expr
        *left, struct const_expr *right);
static struct expr *make_expr(int x, int op, int y, int mult, struct
        const_expr *reloc);
static struct instruction *make_insn_general(struct parse_data *pd, struct
        expr *lhs, int arrow, struct expr *expr);
static struct instruction *make_insn_immediate(struct parse_data *pd, struct
        expr *lhs, struct const_expr *ce);

#define YYLEX_PARAM (pd->scanner)

#define SMALL_IMMEDIATE_BITWIDTH    12
#define LARGE_IMMEDIATE_BITWIDTH    24
#define WORD_BITWIDTH               32

void ce_free(struct const_expr *ce, int recurse);
%}

%error-verbose
%pure-parser
%locations
%define parse.lac full
%lex-param { void *yyscanner }
%parse-param { struct parse_data *pd }
%defines "parser.h"
%output "parser.c"
%name-prefix "tenyr_"
// TODO destructors
//%destructor { insn_free($$); } <insn>
//%destructor { expr_free($$); } <expr>
//%destructor { ce_free($$, 1); } <ce>

%start program

%left EQ NEQ
%left LTE '>'
%left '+' '-'
%left '*' '%'
%left '^' XORN
%left '|' NOR
%left '&' NAND
%left LSH RSH

%token <chr> '[' ']' '.' '(' ')'
%token <chr> '+' '-' '*'
%token <arrow> TOL TOR
%token <str> INTEGER LABEL
%token <chr> REGISTER
%token ILLEGAL WORD

%type <ce> const_expr atom
%type <expr> expr lhs
%type <i> arrow immediate regname
%type <insn> insn data insn_or_data
%type <op> op
%type <program> program
%type <s> addsub
%type <str> lref

%union {
    int32_t i;
    signed s;
    struct const_expr {
        enum { OP2, LAB, IMM, ICI } type;
        int32_t i;
        char labelname[32]; // TODO document length
        int op;
        struct instruction *insn; // for '.'-resolving
        struct const_expr *left, *right;
    } *ce;
    struct expr {
        int deref;
        int x;
        int op;
        int y;
        int32_t i;
        int width;  ///< width of relocation XXX cleanup
        int mult;   ///< multiplier from addsub
        struct const_expr *ce;
    } *expr;
    struct instruction *insn;
    struct instruction_list *program;
    char str[64]; // TODO document length
    char chr;
    int op;
    int arrow;
}

%%

insn_or_data
    : insn
    | data

program[outer]
    : insn_or_data
        {   pd->top = $outer = malloc(sizeof *$outer);
            $outer->next = NULL;
            $outer->insn = $insn_or_data; }
    | insn_or_data program[inner]
        {   pd->top = $outer = malloc(sizeof *$outer);
            $outer->next = $inner;
            $outer->insn = $insn_or_data; }

insn[outer]
    : ILLEGAL
        {   $outer = calloc(1, sizeof *$outer);
            $outer->u.word = -1; }
    | lhs arrow expr
        {   if ($expr->op == OP_RESERVED) {
                if ($arrow == 0) {
                    $outer = make_insn_immediate(pd, $lhs, $expr->ce);
                } else {
                    tenyr_error(&yylloc, pd, "Arrow pointing wrong way");
                    YYERROR;
                }
            } else {
                $outer = make_insn_general(pd, $lhs, $arrow, $expr);
            }
            free($expr);
            free($lhs); }
    | LABEL ':' insn_or_data[inner]
        {   $outer = $inner;
            struct label *n = calloc(1, sizeof *n);
            n->column   = yylloc.first_column;
            n->lineno   = yylloc.first_line;
            n->resolved = 0;
            n->next     = $outer->label;
            strncpy(n->name, $LABEL, sizeof n->name);
            $outer->label = n;

            struct label_list *l = calloc(1, sizeof *l);
            l->next  = pd->labels;
            l->label = n;
            pd->labels = l; }

data
    : WORD const_expr
        {   $data = calloc(1, sizeof *$data);
            add_relocation(pd, $const_expr, 1, &$data->u.word, WORD_BITWIDTH);
            $const_expr->insn = $data; }

lhs[outer]
    : regname { ($outer = malloc(sizeof *$outer))->x = $regname; $outer->deref = 0; }
    /* permits arbitrary nesting, but meaningless */
    | '[' lhs[inner] ']' { $outer = $inner; $outer->deref = 1; }

expr[outer]
    : regname[x]
        { $outer = make_expr($x, OP_BITWISE_OR, 0, 0, NULL); }
    | regname[x] op regname[y]
        { $outer = make_expr($x, $op, $y, 0, NULL); }
    | regname[x] addsub const_expr
        { $outer = make_expr($x, OP_BITWISE_OR, 0, $addsub, $const_expr); }
    | regname[x] op regname[y] addsub const_expr
        { $outer = make_expr($x, $op, $y, $addsub, $const_expr); }
    | const_expr
        { $outer = make_expr(0, OP_RESERVED, 0, 0, $const_expr); }
    | '[' expr[inner] ']' /* TODO lookahead to prevent nesting of [ */
        { $outer = $inner; $outer->deref = 1; }

regname
    : REGISTER { $regname = toupper($REGISTER) - 'A'; }

immediate
    : INTEGER { $immediate = strtol($INTEGER, NULL, 0); }

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

const_expr[outer]
    : atom
        {   $outer = $atom; }
    | const_expr[left] '+' const_expr[right]
        {   $outer = make_const_expr(OP2, '+', $left, $right); }
    | const_expr[left] '-' const_expr[right]
        {   $outer = make_const_expr(OP2, '-', $left, $right); }
    | const_expr[left] '*' const_expr[right]
        {   $outer = make_const_expr(OP2, '*', $left, $right); }
    | '(' const_expr[inner] ')'
        {   $outer = $inner; }

atom
    : immediate
        {   $atom = make_const_expr(IMM, 0, NULL, NULL);
            $atom->i = $immediate; }
    | lref
        {   $atom = make_const_expr(LAB, 0, NULL, NULL);
            strncpy($atom->labelname, $lref, sizeof $atom->labelname); }
    | '.'
        {   $atom = make_const_expr(ICI, 0, NULL, NULL); }

lref
    : '@' LABEL
        { strncpy($lref, $LABEL, sizeof $lref); $lref[sizeof $lref - 1] = 0; }

%%

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s)
{
    fflush(stderr);
    fprintf(stderr, "%s\n", pd->lexstate.saveline);
    fprintf(stderr, "%*s\n%*s on line %d at `%s'\n", locp->last_column, "^",
            locp->last_column, s, locp->first_line, tenyr_get_text(pd->scanner));

    return 0;
}

static struct instruction *make_insn_general(struct parse_data *pd, struct
        expr *lhs, int arrow, struct expr *expr)
{
    struct instruction *insn = calloc(1, sizeof *insn);

    insn->u._0xxx.t  = 0;
    insn->u._0xxx.z  = lhs->x;
    insn->u._0xxx.dd = (lhs->deref << 1) | (expr->deref);
    insn->u._0xxx.x  = expr->x;
    insn->u._0xxx.y  = expr->y;
    insn->u._0xxx.r  = arrow;
    insn->u._0xxx.op = expr->op;
    if (expr->ce) {
        add_relocation(pd, expr->ce, expr->mult, &insn->u.word,
                SMALL_IMMEDIATE_BITWIDTH);
        expr->ce->insn = insn;
    }
    insn->u._0xxx.imm = expr->i;

    return insn;
}

static struct instruction *make_insn_immediate(struct parse_data *pd, struct
        expr *lhs, struct const_expr *ce)
{
    struct instruction *insn = calloc(1, sizeof *insn);

    insn->label = NULL;
    ce->insn = insn;
    add_relocation(pd, ce, 1, &insn->u.word, LARGE_IMMEDIATE_BITWIDTH);
    insn->u._10xx.t  = 2;
    insn->u._10xx.z  = lhs->x;
    insn->u._10xx.dd = lhs->deref << 1;

    return insn;
}

static struct const_expr *add_relocation(struct parse_data *pd, struct
        const_expr *ce, int mult, uint32_t *dest, int width)
{
    struct relocation_list *n = malloc(sizeof *n);

    n->next  = pd->relocs;
    n->ce    = ce;
    n->dest  = dest;
    n->width = width;
    n->mult  = mult;

    pd->relocs = n;

    return ce;
}

static struct const_expr *make_const_expr(int type, int op, struct const_expr
        *left, struct const_expr *right)
{
    struct const_expr *n = malloc(sizeof *n);

    n->type  = type;
    n->op    = op;
    n->left  = left;
    n->right = right;
    n->insn  = NULL;    // top const_expr will have its insn filled in

    return n;
}

static struct expr *make_expr(int x, int op, int y, int mult, struct
        const_expr *reloc)
{
    struct expr *e = malloc(sizeof *e);

    e->deref = 0;
    e->x     = x;
    e->op    = op;
    e->y     = y;
    e->mult  = mult;
    e->ce    = reloc;
    if (reloc)
        e->i = 0xfffffbad; // put in a placeholder that must be overwritten
    else
        e->i = 0; // there was no const_expr ; zero defined by language

    return e;
}

