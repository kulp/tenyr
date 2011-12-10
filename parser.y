%{
    #include <stdio.h>
    #include <stddef.h>

    #include "lexer.h"
    #include "ops.h"

    int yyerror(const char *msg);

    struct instruction_list {
        struct instruction *insn;
        struct instruction_list *next;
    } *top;
%}

%token '[' ']'
%token <op> '|' '&' '+' '*' '%' '^' '>' LSH LTE EQ NOR NAND SUB XORN RSH NEQ
%token TOL TOR
%token <str> INTEGER

%type <op> op /*'|' '&' '+' '*' '%' LSH LTE EQ NOR NAND '^' SUB XORN RSH '>' NEQ*/
%type <insn> insn
%type <program> program
%type <i> immediate
%type <arrow> arrow TOL TOR
%type <expr> expr lhs

%token <i> REGISTER

%union {
    unsigned long i;
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
    int op;     ///< needs to be translated to enum op
    int arrow;
}

%start program

%%

program
    : insn
        {   $$ = malloc(sizeof *$$);
            $$->next = NULL;
            $$->insn = $1; }
    | insn program
        {   $$ = malloc(sizeof *$$);
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
            $$->u._0xxx.imm = $3.i; }
    | lhs TOL immediate
        {   $$ = malloc(sizeof *$$);
            $$->u._10x0.z = $1.x;
            $$->u._10x0.d = $1.deref;
            $$->u._10x0.imm = $3; }

lhs
    : REGISTER
        {   $$.deref = 0;
            $$.x     = $1; }
    | '[' REGISTER ']'
        {   $$.deref = 1;
            $$.x     = $2; }

expr
    : REGISTER op REGISTER '+' immediate
        {   $$.deref = 0;
            $$.x     = $1;
            $$.op    = $2;
            $$.y     = $3;
            $$.i     = $5; }
    | '[' REGISTER op REGISTER '+' immediate ']'
        {   $$.deref = 1;
            $$.x     = $2;
            $$.op    = $3;
            $$.y     = $4;
            $$.i     = $6; }

immediate
    : INTEGER
        { $$ = strtol($1, NULL, 0); }

op
    : '|'
    | '&'
    | '+'
    | '*'
    | '%'
    | LSH
    | LTE
    | EQ
    | NOR
    | NAND
    | '^'
    | SUB
    | XORN
    | RSH
    | '>'
    | NEQ

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

