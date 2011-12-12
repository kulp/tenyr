%{
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "parser_global.h"
#include "lexer.h"

int tenor_error(YYLTYPE *locp, struct parse_data *pd, const char *s);
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

%left LTE EQ NEQ '>'
%left '+' '-'
%left '*' '%'
%left '|' '&' '^' NOR NAND XORN
%left LSH RSH

%token '[' ']' '.' '$'
%token <arrow> TOL TOR
%token <str> INTEGER LABEL
%token <chr> REGISTER
%token ILLEGAL

%type <ce> const_expr add_expr mult_expr atom
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
    struct const_expr {
        enum { ADD, MUL, LAB, IMM, ICI } type;
        uint32_t i;
        uint32_t reladdr : 24;
        char labelname[32]; // TODO document length
        int op;
        struct const_expr *left, *right;
    } ce;
    struct {
        int deref;
        int x;
        int op;
        int y;
        uint32_t i;
        int mult;   ///< multiplier from addsub
        struct const_expr ce;
    } expr;
    struct {
        int sextend;
        uint32_t i;
    } signimm;
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
    | lhs TOL sign_imm
        {   $outer = malloc(sizeof *$outer);
            $outer->u._10x0.p   = $sign_imm.sextend;
            $outer->u._10x0.imm = $sign_imm.i;
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

lhs
    : regname { $lhs.x = $regname; $lhs.deref = 0; }
    /* permits arbitrary nesting, but meaningless */
    | '[' lhs[inner] ']' { $$ = $inner; $$.deref = 1; }

expr[left]
    : regname[x]
        {   $left.deref = 0;
            $left.x     = $x;
            $left.op    = OP_BITWISE_OR;
            $left.y     = 0;
            $left.i     = 0; }
    | regname[x] op regname[y]
        {   $left.deref = 0;
            $left.x     = $x;
            $left.op    = $op;
            $left.y     = $y;
            $left.mult  = 0;
            $left.i     = 0; }
    | regname[x] addsub const_expr
        {   $left.deref = 0;
            $left.x     = $x;
            $left.op    = OP_BITWISE_OR;
            $left.y     = 0;
            $left.mult  = $addsub;
            $left.ce    = $const_expr;
            $left.i     = 0xbad; /*TODO*/}
    | regname[x] op regname[y] addsub const_expr
        {   $left.deref = 0;
            $left.x     = $x;
            $left.op    = $op;
            $left.y     = $y;
            $left.mult  = $addsub;
            $left.ce    = $const_expr;
            $left.i     = 0xbad; /*TODO*/}
    | '[' expr[inner] ']' /* permits arbitrary nesting, but meaningless */
        {   $left = $inner;
            $left.deref = 1; }

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

const_expr
    : add_expr

add_expr
    : mult_expr
    | add_expr '+' mult_expr
    | add_expr '-' mult_expr

mult_expr
    : atom
    | mult_expr '*' atom 

atom
    : immediate
        {   $atom.type = IMM;
            $atom.i = $immediate; }
    | lref
        {   $atom.type = LAB;
            strncpy($atom.labelname, $lref, sizeof $atom.labelname);
        }
    | '.'
        {   $atom.type = ICI;
            $atom.reladdr = pd->reladdr; }

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

