%{
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "debugger_global.h"
#include "debugger_parser.h"
#include "debugger_lexer.h"

int tdbg_error(YYLTYPE *locp, struct debugger_data *dd, const char *s);

#define YYLEX_PARAM (dd->scanner)

%}

%error-verbose
%pure-parser
%locations
%define parse.lac full
%lex-param { void *yyscanner }
%parse-param { struct debugger_data *dd }
%name-prefix "tdbg_"

%start top

%token BREAK CONTINUE PRINT STEPI INTEGER
%token QUIT
%token UNKNOWN
%token NL WHITESPACE
%token REGISTER
%token '*'

%union {
    char chr;
    char str[LINE_LEN];
}

%%

top
    : command-list
    | NL { YYACCEPT; }

command-list
    : /* empty */
    | command maybe-whitespace NL { YYACCEPT; }
    | error NL
        {   yyerrok;
            fputs("Invalid command\n", stdout);
            dd->done = 0;
            YYABORT; }

command
    : PRINT whitespace expr
        { puts("print"); }
    | BREAK whitespace addr-expr
        { puts("break"); }
    | CONTINUE
        { puts("continue"); }
    | STEPI
        { puts("stepi"); }
    | QUIT
        { dd->done = 1; }

maybe-whitespace
    : /* empty */
    | whitespace

whitespace
    : WHITESPACE
    | WHITESPACE whitespace

expr
    : addr-expr
    | INTEGER

addr-expr
    : '*' INTEGER

%%

static char prompt[] = "(tdbg) ";
static size_t prompt_length = sizeof prompt;

int tdbg_prompt(struct debugger_data *dd, FILE *where)
{
    fputs(prompt, where);
    return 0;
}

int tdbg_error(YYLTYPE *locp, struct debugger_data *dd, const char *s)
{
    fflush(stderr);
    fprintf(stderr, "%*s\n%*s at `%s'\n",
            locp->first_column + (int)prompt_length, "^", locp->first_column,
            s, tdbg_get_text(dd->scanner));

    return 0;
}


