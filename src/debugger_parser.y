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

%token BREAK CONTINUE PRINT STEPI INTEGER NL
%token REGISTER
%token '*'

%union {
    char chr;
    char str[LINE_LEN];
}

%%

top
    : command NL
        {   abort(); }

command
    : PRINT expr
    | BREAK addr_expr
    | CONTINUE
    | STEPI

expr
    : addr_expr
    | INTEGER

addr_expr
    : '*' INTEGER

%%

static char prompt[] = "(tdbg) ";
static size_t prompt_length = sizeof prompt;

int tdbg_error(YYLTYPE *locp, struct debugger_data *dd, const char *s)
{
    fflush(stderr);
    fprintf(stderr, "%*s\n%*s at line %d column %d at `%s'\n",
            locp->first_column + (int)prompt_length, "^", locp->first_column +
            (int)prompt_length, s, locp->first_line, locp->first_column +
            (int)prompt_length, tdbg_get_text(dd->scanner));

    return 0;
}


