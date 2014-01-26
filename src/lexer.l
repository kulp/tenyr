%{
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "parser.h"
#include "ops.h"
#include "parser_global.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s);

static int savestr(yyscan_t yyscanner);
static int savechr(int ch, yyscan_t yyscanner);
static int translate_escape(int what);

#define savecol  yyextra->lexstate.savecol
#define saveline yyextra->lexstate.saveline

#define YY_USER_ACTION \
    do { \
        int len = MIN(yyleng, sizeof saveline - savecol); \
        strcopy(&saveline[savecol], yytext, len + 1); \
        saveline[savecol += len] = 0; \
        yylloc->first_line = yylloc->last_line = yylineno; \
        yylloc->first_column = yycolumn; \
        yylloc->last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng; \
    } while (0);

#define YSTR        yylval->str
#define PUSH(State) yy_push_state(State, yyscanner)
#define DUP         yy_push_state(yy_top_state(yyscanner), yyscanner)
#define POP         yy_pop_state(yyscanner)
#define RESETSTR    YSTR.pos = YSTR.len = 0
#define DONESTR     do { YSTR.len = YSTR.pos; YSTR.pos = 0; } while (0)

%}

%option reentrant bison-bridge bison-locations stack
%option noyywrap never-interactive yylineno batch
%option case-insensitive
%option extra-type="struct parse_data *"
%option prefix="tenyr_"

%s ignore_nl
%s needarrow
%x charlit charend charesc
%x comment linecomment
%x string

regname [A-P]
/* don't permit collision with regname */
symbol  [A-Z_][A-Z0-9_]{1,30}|[Q-Z_][A-Z0-9_]{0,30}
/* local symbols start with ".L" */
local   \.L[A-Z0-9_]{1,31}
string  ([^"]|\\\")+
hexnum  "0x"[0-9a-f_]+
octnum  "0"[0-7_]+
binnum  "0b"[01_]+
decnum  [0-9_]+
bitstr  {hexnum}|{octnum}|{binnum}
self    [][|&+*^<>.,:@()~;/-]
strcon  ([^"]|\\\")
escchar [\\"'0nbrtfv]
white   [\t\f\v\r ]

%%

\t                  { yycolumn += 7; /* 1 added in YY_USER_ACTION */ }
illegal             { return ILLEGAL; }

{regname}           { yylval->chr = yytext[0]; return REGISTER; }
{symbol}            { RESETSTR; savestr(yyscanner); return SYMBOL; }
{local}             { RESETSTR; savestr(yyscanner); return LOCAL; }

\"                  { RESETSTR; PUSH(string); }
<string>{
    /* avoid returning STRING multiple times ; build a string here instead */
    \"              { POP; DONESTR; return STRING; }
    \\              { DUP; /* because charesc double-POPs */ PUSH(charesc); }
    [^\\"]+         savestr(yyscanner);
}

".word"{white}      { PUSH(ignore_nl); return WORD; }
".ascii"{white}     { PUSH(ignore_nl); return ASCII; }
".utf32"{white}     { PUSH(ignore_nl); return UTF32; }
".global"{white}    { PUSH(ignore_nl); return GLOBAL; }
".set"{white}       { PUSH(ignore_nl); return SET; }

{self}              { return yytext[0]; }

"<<"                { return LSH;  }
"=="                { return EQ;   }
"&~"                { return ANDN; }
"<>"                { return NEQ;  }
"^~"                { return XORN; }
">>"                { return RSH;  }
<needarrow>"<-"     { return TOL;  }
"->"                { return TOR;  }

{hexnum}|{octnum}   { yylval->i = numberise(&yytext[0],  0); return BITSTRING; }
{binnum}            { yylval->i = numberise(&yytext[2],  2); return BITSTRING; }
{decnum}            { yylval->i = numberise(&yytext[0], 10); return INTEGER; }

{white}+            ; /* ignored whitespace */

"//"|"#"            PUSH(linecomment);
<linecomment>{
    \\\r?\n         ; /* escaped newline */
    \r?\n           { POP; return '\n'; }
    .               ; /* ignored content */
}

"/*"                PUSH(comment);
<comment>{
    "*/"            POP; /* pop comment, reveal INITIAL */
    .               ; /* ignored content */
}

"'"                 { PUSH(charend); PUSH(charlit); DONESTR; }
<charlit>{
    \\              PUSH(charesc);
    .               { savechr(yytext[0], yyscanner); POP; }
}
<charesc>{
    \r?\n           POP; /* escaped newline */
    {escchar}       { savechr(translate_escape(yytext[0]), yyscanner); POP; POP; }
    .               { tenyr_error(yylloc, yyextra, "bad escape character"); POP; }
}
<charend>"'"        { POP; return CHARACTER; }

<*>\n               { saveline[savecol = 0] = 0; return '\n'; }

<*>.                tenyr_error(yylloc, yyextra, "unexpected character");

%%

static int savestr(yyscan_t yyscanner)
{
    // savestr() appends a string piece into a buffer
    int len          = yyget_leng(yyscanner);
    YYSTYPE *lval    = yyget_lval(yyscanner);
    YYLTYPE *lloc    = yyget_lloc(yyscanner);
    const char *text = yyget_text(yyscanner);
    void *extra      = yyget_extra(yyscanner);
    struct strbuf *s = &lval->str;

    if (s->pos + len >= (signed)sizeof s->buf) {
        tenyr_error(lloc, extra, "token too long");
        return 1;
    } else {
        memcpy(&s->buf[s->pos], text, len + 1);
        s->pos += len;
        s->len += len;
        return 0;
    }
}

static int savechr(int ch, yyscan_t yyscanner)
{
    // savechr() appends a character into the string-building buffer
    int len          = 1;
    YYSTYPE *lval    = yyget_lval(yyscanner);
    struct strbuf *s = &lval->str;

    s->buf[s->pos++] = ch;
    s->len++;

    return 0;
}

static int translate_escape(int what)
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
        default: return -1;
    }
}

void tenyr_push_state(int st, void *yyscanner)
{
    PUSH(st);
}

void tenyr_pop_state(void *yyscanner)
{
    POP;
}

