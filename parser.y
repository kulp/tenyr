%{
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "parser_global.h"
#include "lexer.h"

int yyerror(YYLTYPE *locp, struct parse_data *pd, const char *s);
#define YYLEX_PARAM (pd->scanner)
%}

%error-verbose
%pure-parser
%locations
%define parse.lac full
%lex-param { void *yyscanner }
%parse-param { struct parse_data *pd }

%start program

%token '[' ']' '.'
%token '|' '&' '+' '-' '*' '%' '^' '>' LSH LTE EQ NOR NAND XORN RSH NEQ '$'
%token <arrow> TOL TOR
%token <str> INTEGER LABEL
%token <chr> REGISTER
%token ILLEGAL

%type <ce> const_expr add_expr mult_expr const_atom
%type <expr> expr lhs
%type <i> arrow immediate regname
%type <insn> insn
%type <op> op
%type <program> program
%type <s> addsub
%type <signimm> sign_immediate
%type <str> labelref

%union {
    unsigned long i;
    signed s;
    struct const_expr {
        enum { ADD, MUL, LAB, IMM, ICI } type;
        unsigned long i;
        unsigned long reladdr;
        char labelname[32]; // TODO document length
        int op;
        struct const_expr *left, *right;
    } ce;
    struct {
        int deref;
        int x;
        int op;
        int y;
        unsigned long i;
        int mult;   ///< multiplier from addsub
        struct const_expr ce;
    } expr;
    struct {
        int sextend;
        unsigned long i;
    } signimm;
    struct label *label;
    struct instruction *insn;
    struct instruction_list *program;
    char str[64]; // TODO document length
    char chr;
    int op;
    int arrow;
}

%%

program
    : insn
        {   pd->top = $$ = malloc(sizeof *$$);
            $$->next = NULL;
            $$->insn = $1; }
    | insn program
        {   pd->top = $$ = malloc(sizeof *$$);
            pd->reladdr++; // XXX not safe ? when does this happen ?
            $$->next = $2;
            $$->insn = $1; }

insn
    : ILLEGAL
        {   $$ = malloc(sizeof *$$);
            $$->u.word = -1; }
    | lhs arrow expr
        {   $$ = malloc(sizeof *$$);
            $$->u._0xxx.t   = 0;
            $$->u._0xxx.z   = $1.x;
            $$->u._0xxx.dd  = ($1.deref << 1) | ($3.deref);
            $$->u._0xxx.x   = $3.x;
            $$->u._0xxx.y   = $3.y;
            $$->u._0xxx.r   = $2;
            $$->u._0xxx.op  = $3.op;
            $$->u._0xxx.imm = $3.i; }
    | lhs TOL sign_immediate
        {   $$ = malloc(sizeof *$$);
            $$->u._10x0.p   = $3.sextend;
            $$->u._10x0.imm = $3.i;
            $$->u._10x0.t   = 2;
            $$->u._10x0.z   = $1.x;
            $$->u._10x0.d   = $1.deref; }
    | LABEL ':' insn
        {   // TODO add label to a chain, and associate it with the
            // instruction
            $$ = $3;
            struct label *n = malloc(sizeof *n);
            n->column   = yylloc.first_column;
            n->lineno   = yylloc.first_line;
            n->resolved = 1;
            n->reladdr  = pd->reladdr;
            n->next     = $$->label;
            strncpy(n->name, $1, sizeof n->name);
            $$->label = n;

            struct label_list *l = malloc(sizeof *l);
            l->next  = pd->labels;
            l->label = n;
            pd->labels = l;
        }

lhs
    : regname { $$.deref = 0; $$.x = $1; }
    /* permits arbitrary nesting, but meaningless */
    | '[' lhs ']' { $$ = $2; $$.deref = 1; }

expr
    : regname
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = OP_BITWISE_OR;
            $$.y     = 0;
            $$.i     = 0; }
    | regname op regname
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = $2;
            $$.y     = $3;
            $$.mult  = 0;
            $$.i     = 0; }
    | regname addsub const_expr
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = OP_BITWISE_OR;
            $$.y     = 0;
            $$.mult  = $2;
            $$.ce    = $3;
            $$.i     = 0xbad; }
    | regname op regname addsub const_expr
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = $2;
            $$.y     = $3;
            $$.mult  = $4;
            $$.ce    = $5;
            $$.i     = 0xbad; }
    | '[' expr ']' /* permits arbitrary nesting, but meaningless */
        {   $$ = $2;
            $$.deref = 1; }

regname
    : REGISTER { $$ = toupper($1) - 'A'; }

sign_immediate
    : immediate     { $$.sextend = 0; $$.i = $1; }
    | '$' immediate { $$.sextend = 1; $$.i = $2; }

immediate
    : INTEGER { $$ = strtol($1, NULL, 0); }

addsub
    : '+' { $$ =  1; }
    | '-' { $$ = -1; }

op
    : '|'   { $$ = OP_BITWISE_OR         ; }
    | '&'   { $$ = OP_BITWISE_AND        ; }
    | '+'   { $$ = OP_ADD                ; }
    | '*'   { $$ = OP_MULTIPLY           ; }
    | '%'   { $$ = OP_MODULUS            ; }
    | LSH   { $$ = OP_SHIFT_LEFT         ; }
    | LTE   { $$ = OP_COMPARE_LTE        ; }
    | EQ    { $$ = OP_COMPARE_EQ         ; }
    | NOR   { $$ = OP_BITWISE_NOR        ; }
    | NAND  { $$ = OP_BITWISE_NAND       ; }
    | '^'   { $$ = OP_BITWISE_XOR        ; }
    | '-'   { $$ = OP_ADD_NEGATIVE_Y     ; }
    | XORN  { $$ = OP_XOR_INVERT_X       ; }
    | RSH   { $$ = OP_SHIFT_RIGHT_LOGICAL; }
    | '>'   { $$ = OP_COMPARE_GT         ; }
    | NEQ   { $$ = OP_COMPARE_NE         ; }

arrow
    : TOL { $$ = 0; }
    | TOR { $$ = 1; }

const_expr
    : add_expr

add_expr
    : mult_expr
    | add_expr '+' mult_expr
    | add_expr '-' mult_expr

mult_expr
    : const_atom
    | mult_expr '*' const_atom 

const_atom
    : immediate
        {   $$.type = IMM;
            $$.i = $1; }
    | labelref
        {   $$.type = LAB;
            strncpy($$.labelname, $1, sizeof $$.labelname);
        }
    | '.'
        {   $$.type = ICI;
            $$.reladdr = pd->reladdr; }

labelref
    : '@' LABEL { strncpy($$, $2, sizeof $$); $$[sizeof $$ - 1] = 0; }

%%

int yyerror(YYLTYPE *locp, struct parse_data *pd, const char *s)
{
    fflush(stderr);
    YYLTYPE *loc = yyget_lloc(pd->scanner);
    fprintf(stderr, "%*s\n%*s on line %d at `%s'\n", loc->last_column, "^",
            loc->last_column, s, locp->first_line, yyget_text(pd->scanner));

    return 0;
}

