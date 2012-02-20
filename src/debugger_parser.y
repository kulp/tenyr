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

%type <expr> expr addr_expr
%type <i32> integer regname
%type <cmd> command display_command info_command print_command
%type <chr> format

%token STEPI DISPLAY INFO PRINT
%token <str> INTEGER IDENT
%token UNKNOWN
%token NL WHITESPACE
%token <chr> REGISTER
%token '*' '/'
%token <chr> 'i' 'd' 'x'

%union {
    char chr;
    char str[LINE_LEN];
    int32_t i32;
    struct debug_expr expr; // TODO support proper constant expression trees
    struct debug_cmd cmd;
}

%%

top
    : /* empty */
    | maybe_command

maybe_command
    : NL { YYACCEPT; }
    | maybe_whitespace command maybe_whitespace NL { dd->cmd = $command; YYACCEPT; }
    | error NL
        {   yyerrok;
            fputs("Invalid command\n", stderr);
            dd->cmd.code = CMD_NULL;
            YYABORT; }

command
    : 'b' whitespace addr_expr
        {   $command.code = CMD_SET_BREAKPOINT;
            $command.arg.expr = $addr_expr; }
    | 'd' whitespace addr_expr
        {   $command.code = CMD_DELETE_BREAKPOINT;
            $command.arg.expr = $addr_expr; }
    | 'c'
        {   $command.code = CMD_CONTINUE; }
    | STEPI
        {   $command.code = CMD_STEP_INSTRUCTION; }
    | 'q'
        {   $command.code = CMD_QUIT; }
    | print_command
    | display_command
    | info_command

display_command
    : DISPLAY whitespace expr
        {   $display_command.code = CMD_DISPLAY;
            $display_command.arg.fmt = DISP_NULL;
            $display_command.arg.expr = $expr; }
    | DISPLAY '/' format whitespace expr
        {   $display_command.code = CMD_DISPLAY;
            $display_command.arg.fmt = $format;
            $display_command.arg.expr = $expr; }

print_command
    : print whitespace expr
        {   $print_command.code = CMD_PRINT;
            $print_command.arg.fmt = DISP_NULL;
            $print_command.arg.expr = $expr; }
    | print '/' format whitespace expr
        {   $print_command.code = CMD_PRINT;
            $print_command.arg.fmt = $format;
            $print_command.arg.expr = $expr; }

print
    : 'p'
    | PRINT

info_command
    : INFO whitespace IDENT
        {   $info_command.code = CMD_GET_INFO;
            strncpy($info_command.arg.str,
                    $IDENT,
                    sizeof $info_command.arg.str - 1);
            $info_command.arg.str[sizeof $info_command.arg.str - 1] = 0; }

format
    : 'i' { $format = DISP_INST; }
    | 'd' { $format = DISP_DEC; }
    | 'x' { $format = DISP_HEX; }

maybe_whitespace
    : /* empty */
    | whitespace

whitespace
    : WHITESPACE
    | WHITESPACE whitespace

expr
    : regname
        {   $expr.val = $regname;
            $expr.type = EXPR_REG; }
    | addr_expr

regname
    : REGISTER
        { $regname = $REGISTER - 'A'; }

addr_expr
    : '*' integer
        {   $addr_expr.val = $integer;
            $addr_expr.type = EXPR_MEM; }

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


