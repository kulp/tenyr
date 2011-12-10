%{
    #include <stdio.h>
    #include <stdint.h>

    int yyerror (char const *);
    #include "lexer.h"

    struct program *top;
%}

%token '[' ']'
%token <op> '|' '&' '+' '*' '%' '^' '>' LSH LTE EQ NOR NAND SUB XORN RSH NEQ
%token TOL TOR

%type <op> op

%token INTEGER REGISTER

%union {
    uint32_t i;
    char *str;
    char chr;
    int op;     ///< needs to be translated to enum op
    int arrow;
}

%start program

%%

program
    : expr
    | expr program

expr
    : lhs arrow rhs

lhs
    :     REGISTER
    | '[' REGISTER ']'

rhs
    :     REGISTER op REGISTER '+' immediate
    | '[' REGISTER op REGISTER '+' immediate ']'

immediate
    : INTEGER

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
    fprintf(stderr, "Error on line %d at `%s' : \n", lineno + 1, yytext); // zero-based
    fprintf(stderr, "%*s\n%*s\n", column, "^", column, s);

    return 0;
}

struct program *tenor_get_parser_result(void)
{
    return top;
}

