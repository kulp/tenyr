%{
    #include <stdio.h>
    #include <stddef.h>
    #include <ctype.h>

    #include "parser_global.h"
    #include "lexer.h"
    #include "ops.h"

    int yyerror(const char *msg);
    struct instruction_list *top;
%}

%token '[' ']'
%token '|' '&' '+' '-' '*' '%' '^' '>' LSH LTE EQ NOR NAND XORN RSH NEQ
%token TOL TOR
%token <str> INTEGER
%token <i> REGISTER

%type <op> op
%type <insn> insn
%type <program> program
%type <i> immediate
%type <arrow> arrow TOL TOR
%type <expr> expr lhs
%type <s> addsub
%type <i> regname

%union {
    unsigned long i;
    signed s;
    struct {
        int deref;
        int x;
        int op;
        int y;
        unsigned long i;
    } expr;
    struct instruction *insn;
    struct instruction_list *program;
    char *str;
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
            $$->next = $2;
            $$->insn = $1; }

insn
    : lhs arrow expr
        {   $$ = malloc(sizeof *$$);
            $$->u._0xxx.z   = $1.x;
            $$->u._0xxx.dd  = ($1.deref << 1) | ($3.deref);
            $$->u._0xxx.x   = $3.x;
            $$->u._0xxx.y   = $3.y;
            $$->u._0xxx.r   = $2 == TOR;
            $$->u._0xxx.op  = $3.op;
            $$->u._0xxx.imm = $3.i; }
    | lhs TOL immediate
        {   $$ = malloc(sizeof *$$);
            $$->u._10x0.z = $1.x;
            $$->u._10x0.d = $1.deref;
            $$->u._10x0.imm = $3; }

lhs
    : regname
        {   $$.deref = 0;
            $$.x     = $1; }
    | '[' regname ']'
        {   $$.deref = 1;
            $$.x     = $2; }

expr
    : regname op regname
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = $2;
            $$.y     = $3;
            $$.i     = 0; }
    | regname addsub immediate
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = OP_BITWISE_OR;
            $$.y     = 0;
            $$.i     = $2 * $3; }
    | regname op regname addsub immediate
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = $2;
            $$.y     = $3;
            $$.i     = $4 * $5; }
    | '[' regname op regname addsub immediate ']'
        {   $$.deref = 1;
            $$.x     = $2;
            $$.op    = $3;
            $$.y     = $4;
            $$.i     = $5 * $6; }

regname
    : REGISTER { $$ = toupper($1) - 'A'; }

immediate
    : INTEGER
        { $$ = strtol($1, NULL, 0); }

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
    : TOL
    | TOR

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

