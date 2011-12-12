%{
    #include <stdio.h>
    #include <stddef.h>
    #include <ctype.h>
    #include <stdint.h>

    #include "parser_global.h"
    #include "lexer.h"
    #include "ops.h"

    int yyerror(const char *msg);
    static struct instruction_list *top;
    static struct label_list {
        struct label *label;
        struct label_list *next;
    } *labels;
    static uint32_t reladdr;
%}

%error-verbose

%token '[' ']' '.'
%token '|' '&' '+' '-' '*' '%' '^' '>' LSH LTE EQ NOR NAND XORN RSH NEQ '$'
%token <arrow> TOL TOR
%token <str> INTEGER LABEL
%token <i> REGISTER
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

%start program

%%

program
    : insn
        {   top = $$ = malloc(sizeof *$$);
            $$->next = NULL;
            $$->insn = $1; }
    | insn program
        {   top = $$ = malloc(sizeof *$$);
            reladdr++; // XXX not safe ? when does this happen ?
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
            extern int column, lineno;
            struct label *n = malloc(sizeof *n);
            n->column   = column;
            n->lineno   = lineno;
            n->resolved = 1;
            n->reladdr  = reladdr;
            n->next     = $$->label;
            strncpy(n->name, $1, sizeof n->name);
            $$->label = n;

            struct label_list *l = malloc(sizeof *l);
            l->next  = labels;
            l->label = n;
            labels = l;
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
            $$.ce    = $3; }
    | regname op regname addsub const_expr
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = $2;
            $$.y     = $3;
            $$.mult  = $4;
            $$.ce    = $5; }
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
            $$.reladdr = reladdr; }

labelref
    : '@' LABEL { strncpy($$, $2, sizeof $$); $$[sizeof $$ - 1] = 0; }

%%

int yyerror(const char *s)
{
    extern int lineno, column;
    extern char *yytext;

    fflush(stderr);
    fprintf(stderr, "%*s\n%*s on line %d at `%s'\n", column, "^", column, s,
            lineno + 1, yytext);

    return 0;
}

struct instruction_list *tenor_get_parser_result(void)
{
    return top;
}

