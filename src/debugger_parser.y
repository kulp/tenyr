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

%type <expr> expr
%type <i32> addr_expr integer

%token BREAK DELETE CONTINUE PRINT STEPI
%token <str> INTEGER
%token QUIT
%token UNKNOWN
%token NL WHITESPACE
%token REGISTER
%token '*'

%union {
    char chr;
    char str[LINE_LEN];
    int32_t i32;
    int32_t expr; // TODO support proper constant expression trees
}

%%

top
    : /* empty */
    | maybe_command

maybe_command
    : NL { YYACCEPT; }
    | command maybe_whitespace NL { YYACCEPT; }
    | error NL
        {   yyerrok;
            fputs("Invalid command\n", stdout);
            dd->cmd.code = CMD_NULL;
            YYABORT; }

command
    : PRINT whitespace expr
        {   dd->cmd.code = CMD_PRINT;
            dd->cmd.arg = $expr; }
    | BREAK whitespace addr_expr
        {   dd->cmd.code = CMD_SET_BREAKPOINT;
            dd->cmd.arg = $addr_expr; }
    | DELETE whitespace addr_expr
        {   dd->cmd.code = CMD_DELETE_BREAKPOINT;
            dd->cmd.arg = $addr_expr; }
    | CONTINUE
        {   dd->cmd.code = CMD_CONTINUE; }
    | STEPI
        {   dd->cmd.code = CMD_STEP_INSTRUCTION; }
    | QUIT
        {   dd->cmd.code = CMD_QUIT; }

maybe_whitespace
    : /* empty */
    | whitespace

whitespace
    : WHITESPACE
    | WHITESPACE whitespace

expr
    : addr_expr
        { $expr = $addr_expr; /* TODO */ }
    | integer
        { $expr = $integer; /* TODO */ }

addr_expr
    : '*' integer
        { $addr_expr = $integer; }

integer
    : INTEGER
        { $integer = strtol($INTEGER, NULL, 0); }

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


