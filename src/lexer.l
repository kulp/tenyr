%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"
#include "parser.h"
#include "ops.h"
#include "parser_global.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s, ...);

static struct cstr *resizestr(struct cstr *s, int len);
static int savestr(const char *text, int len, yyscan_t yyscanner);
static int savechr(char ch, yyscan_t yyscanner) { return savestr(&ch, 1, yyscanner); }
static int translate_escape(YYLTYPE *loc, struct parse_data *extra, int what);

#define savecol  (yyextra->lexstate.savecol)
#define saveline (yyextra->lexstate.saveline)

#define YY_USER_ACTION \
    do { \
        size_t len = MIN((size_t)yyleng, (size_t)(LINE_LEN - savecol)); \
        strcopy(&saveline[savecol], yytext, len + 1); \
        saveline[savecol += len] = 0; \
        yylloc->first_line = yylloc->last_line = yylineno; \
        yylloc->first_column = yycolumn; \
        yylloc->last_column = (int)(yycolumn + yyleng - 1); \
        yycolumn += yyleng; \
    } while (0);

#define PUSH(State) yy_push_state(State, yyscanner)
#define DUP         yy_push_state(yy_top_state(yyscanner), yyscanner)
#define POP         yy_pop_state(yyscanner)
#define SAVESTR     savestr(yytext, yyleng, yyscanner)

%}

%option reentrant bison-bridge bison-locations stack
%option noyywrap yylineno batch
%option case-insensitive
%option extra-type="struct parse_data *"
%option prefix="tenyr_"

%s ignore_nl
%s needarrow
%x charlit charend charesc
%x linecomment
%x string

regname [A-P]
/* don't permit collision with regname */
symbol  [A-Z_][A-Z0-9_]+|[Q-Z_][A-Z0-9_]*
/* local symbols start with ".L" */
local   \.L[A-Z0-9_]+
string  ([^"]|\\\")+
hexnum  "0x"[0-9a-f_]+
binnum  "0b"[01_]*[01]
decnum  0|[1-9]([0-9_]*[0-9])?
bitstr  {hexnum}|{binnum}
self    [][|&+*^><.,:@()~;=/-]
strcon  ([^"]|\\\")
escchar [\\"'0nbrtfv]
white   [\t\f\v\r ]
edge    {white}|\n

%%

\t                  { yycolumn += 7; /* 1 added in YY_USER_ACTION */ }
illegal             { return ILLEGAL; }

{regname}           { yylval->i = yytext[0]; return REGISTER; }
{symbol}            { yylval->cstr = NULL; SAVESTR; return SYMBOL; }
{local}             { yylval->cstr = NULL; SAVESTR; return LOCAL; }

\"                  { yylval->cstr = NULL; PUSH(string); return DBLQUOTE; }
<string>{
    \"              { POP; return DBLQUOTE; }
    \\{escchar}     { savechr(translate_escape(yylloc, yyextra, yytext[1]), yyscanner); return STRSPAN; }
    [^\\"]+         { SAVESTR; return STRSPAN; }
}

".word"{edge}       { PUSH(ignore_nl); return WORD; }
".chars"{edge}      { PUSH(ignore_nl); return CHARS; }
".global"{edge}     { PUSH(ignore_nl); return GLOBAL; }
".set"{edge}        { PUSH(ignore_nl); return SET; }
".zero"{edge}       { PUSH(ignore_nl); return ZERO; }

{self}              { return yytext[0]; }

"<<<"               { return LSH;  }
"<<"                { return LSH;  }
">="                { return GE;   }
"<="                { return LE;   }
"=="                { return EQ;   }
"&~"                { return ANDN; }
"|~"                { return ORN;  }
">>"                { return RSHA; }
">>>"               { return RSH;  }
"^^"                { return PACK; }
<needarrow>"<-"     { return TOL;  }
"->"                { return TOR;  }

{hexnum}            { yylval->i = numberise(&yytext[0], 16); return BITSTRING; }
{binnum}            { yylval->i = numberise(&yytext[2],  2); return BITSTRING; }
{decnum}            { yylval->i = numberise(&yytext[0], 10); return INTEGER; }

{white}+            ; /* ignored whitespace */

"#"                 PUSH(linecomment);
<linecomment>{
    \r?\n           { POP; return '\n'; }
    .               ; /* ignored content */
}

"'"                 { PUSH(charend); PUSH(charlit); yylval->cstr = NULL; }
<charlit>{
    \\              PUSH(charesc);
    .               { savechr(yytext[0], yyscanner); POP; }
}
<charesc>{
    \r?\n           POP; /* escaped newline */
    {escchar}       { savechr(translate_escape(yylloc, yyextra, yytext[0]), yyscanner); POP; POP; }
    .               { tenyr_error(yylloc, yyextra, "bad escape character"); POP; }
}
<charend>"'"        { POP; return CHARACTER; }

<*>\n               { savecol = 0; yycolumn = 0; return '\n'; }

<*>.                tenyr_error(yylloc, yyextra, "unexpected character `%c`", yytext[0]);

%%

static int savestr(const char *text, int len, yyscan_t yyscanner)
{
    // savestr() appends a string piece into a buffer
    YYSTYPE *lval  = yyget_lval(yyscanner);
    struct cstr *s = lval->cstr;

    s = malloc(sizeof *s);
    s->size = len;
    s->head = s->tail = calloc((size_t)s->size, 1);
    s->last = s;
    s->right = NULL;

    memcpy(s->tail, text, len);
    s->tail += len;

    lval->cstr = s;

    return 0;
}

static int translate_escape(YYLTYPE *lloc, struct parse_data *extra, int what)
{
    switch (what) {
        case '\\': return '\\';
        case '\'': return '\'';
        case '"' : return '\"';
        case '0' : return '\0';
        case 'b' : return '\b';
        case 'f' : return '\f';
        case 'n' : return '\n';
        case 'r' : return '\r';
        case 't' : return '\t';
        case 'v' : return '\v';
        default:
            tenyr_error(lloc, extra, "invalid escape \\%c - did you mean \\%c ?",
                        what, tolower(what));
            return -1;
    }
}

void tenyr_push_state(int st, void *yyscanner) { PUSH(st); }
void tenyr_pop_state (        void *yyscanner) { POP;      }

