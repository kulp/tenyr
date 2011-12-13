%{
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "parser_global.h"
#include "lexer.h"

int tenor_error(YYLTYPE *locp, struct parse_data *pd, const char *s);
static struct const_expr *add_relocation(struct parse_data *pd, struct
        const_expr *ce);
static struct const_expr *make_const_expr(int type, struct const_expr *left,
        struct const_expr *right);

#define YYLEX_PARAM (pd->scanner)
%}

%error-verbose
%pure-parser
%locations
%define parse.lac full
%lex-param { void *yyscanner }
%parse-param { struct parse_data *pd }
%defines "parser.h"
%output "parser.c"
%name-prefix "tenor_"
// %destructor { free ($$); } <*> // TODO

%start program

%left EQ NEQ
%left LTE '>'
%left '+' '-'
%left '*' '%'
%left '^' XORN
%left '|' NOR
%left '&' NAND
%left LSH RSH

%token '[' ']' '.' '$' '(' ')'
%token <arrow> TOL TOR
%token <str> INTEGER LABEL
%token <chr> REGISTER
%token ILLEGAL

%type <ce> const_expr atom
%type <expr> expr lhs
%type <i> arrow immediate regname
%type <insn> insn
%type <op> op
%type <program> program
%type <s> addsub
%type <signimm> sign_imm
%type <str> lref

%union {
    uint32_t i;
    signed s;
    struct sign_imm {
        int sextend;
        uint32_t i;
    } signimm;
    struct const_expr {
        enum { ADD, SUB, MUL, LAB, IMM, ICI, PAR } type;
        struct sign_imm i;
        uint32_t reladdr : 24;
        char labelname[32]; // TODO document length
        int op;
        struct const_expr *left, *right;
    } *ce;
    struct {
        int deref;
        int x;
        int op;
        int y;
        uint32_t i;
        int mult;   ///< multiplier from addsub
        struct const_expr *ce;
    } expr;
    struct instruction *insn;
    struct instruction_list *program;
    char str[64]; // TODO document length
    char chr;
    int op;
    int arrow;
}

%%

program[outer]
    : insn
        {   pd->top = $outer = malloc(sizeof *$outer);
            $outer->next = NULL;
            $outer->insn = $insn; }
    | insn program[inner]
        {   pd->top = $outer = malloc(sizeof *$outer);
            pd->reladdr++; // XXX not safe ? when does this happen ?
            $outer->next = $inner;
            $outer->insn = $insn; }

insn[outer]
    : ILLEGAL
        {   $outer = malloc(sizeof *$outer);
            $outer->u.word = -1; }
    | lhs arrow expr
        {   $outer = malloc(sizeof *$outer);
            $outer->u._0xxx.t   = 0;
            $outer->u._0xxx.z   = $lhs.x;
            $outer->u._0xxx.dd  = ($lhs.deref << 1) | ($lhs.deref);
            $outer->u._0xxx.x   = $expr.x;
            $outer->u._0xxx.y   = $expr.y;
            $outer->u._0xxx.r   = $arrow;
            $outer->u._0xxx.op  = $expr.op;
            $outer->u._0xxx.imm = $expr.i; }
    | lhs TOL const_expr
        {   $outer = malloc(sizeof *$outer);
            /*
            $outer->u._10x0.p   = $sign_imm.sextend;
            $outer->u._10x0.imm = $sign_imm.i;
            */ // TODO
            add_relocation(pd, $const_expr);
            $outer->u._10x0.t   = 2;
            $outer->u._10x0.z   = $lhs.x;
            $outer->u._10x0.d   = $lhs.deref; }
    | LABEL ':' insn[inner]
        {   // TODO add label to a chain, and associate it with the
            // instruction
            $outer = $inner;
            struct label *n = malloc(sizeof *n);
            n->column   = yylloc.first_column;
            n->lineno   = yylloc.first_line;
            n->resolved = 1;
            n->reladdr  = pd->reladdr;
            n->next     = $outer->label;
            strncpy(n->name, $LABEL, sizeof n->name);
            $outer->label = n;

            struct label_list *l = malloc(sizeof *l);
            l->next  = pd->labels;
            l->label = n;
            pd->labels = l;
        }

lhs[outer]
    : regname { $outer.x = $regname; $outer.deref = 0; }
    /* permits arbitrary nesting, but meaningless */
    | '[' lhs[inner] ']' { $outer = $inner; $outer.deref = 1; }

expr[outer]
    : regname[x]
        {   $outer.deref = 0;
            $outer.x     = $x;
            $outer.op    = OP_BITWISE_OR;
            $outer.y     = 0;
            $outer.i     = 0; }
    | regname[x] op regname[y]
        {   $outer.deref = 0;
            $outer.x     = $x;
            $outer.op    = $op;
            $outer.y     = $y;
            $outer.mult  = 0;
            $outer.i     = 0; }
    | regname[x] addsub const_expr
        {   $outer.deref = 0;
            $outer.x     = $x;
            $outer.op    = OP_BITWISE_OR;
            $outer.y     = 0;
            $outer.mult  = $addsub;
            $outer.ce    = add_relocation(pd, $const_expr);
            $outer.i     = 0xbad; /*TODO*/}
    | regname[x] op regname[y] addsub const_expr
        {   $outer.deref = 0;
            $outer.x     = $x;
            $outer.op    = $op;
            $outer.y     = $y;
            $outer.mult  = $addsub;
            $outer.ce    = add_relocation(pd, $const_expr);
            $outer.i     = 0xbad; /*TODO*/}
    | '[' expr[inner] ']' /* permits arbitrary nesting, but meaningless */
        {   $outer = $inner;
            $outer.deref = 1; }

regname
    : REGISTER { $regname = toupper($REGISTER) - 'A'; }

sign_imm
    : immediate     { $sign_imm.i = $immediate; $sign_imm.sextend = 0; }
    | '$' immediate { $sign_imm.i = $immediate; $sign_imm.sextend = 1; }

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
    | '%'   { $op = OP_MODULUS            ; }
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
        {   $outer = make_const_expr(ADD, $left, $right); }
    | const_expr[left] '-' const_expr[right]
        {   $outer = make_const_expr(SUB, $left, $right); }
    | const_expr[left] '*' const_expr[right]
        {   $outer = make_const_expr(MUL, $left, $right); }
    | '(' const_expr[inner] ')'
        {   $outer = make_const_expr(PAR, $inner, NULL); }

atom
    : sign_imm
        {   $atom = make_const_expr(IMM, NULL, NULL);
            $atom->i = $sign_imm; }
    | lref
        {   $atom = make_const_expr(LAB, NULL, NULL);
            strncpy($atom->labelname, $lref, sizeof $atom->labelname);
        }
    | '.'
        {   $atom = make_const_expr(ICI, NULL, NULL);
            $atom->reladdr = pd->reladdr; }

lref
    : '@' LABEL
        { strncpy($lref, $LABEL, sizeof $lref); $lref[sizeof $lref - 1] = 0; }

%%

int tenor_error(YYLTYPE *locp, struct parse_data *pd, const char *s)
{
    fflush(stderr);
    fprintf(stderr, "%*s\n%*s on line %d at `%s'\n", locp->last_column, "^",
            locp->last_column, s, locp->first_line, tenor_get_text(pd->scanner));

    return 0;
}

static struct const_expr *add_relocation(struct parse_data *pd, struct const_expr *ce)
{
    struct relocation_list *n = malloc(sizeof *n);

    n->next = pd->relocs;
    n->ce = ce;
    pd->relocs = n;

    return ce;
}

static struct const_expr *make_const_expr(int type, struct const_expr *left,
        struct const_expr *right)
{
    struct const_expr *n = malloc(sizeof *n);

    n->type  = type;
    n->left  = left;
    n->right = right;

    return n;
}
