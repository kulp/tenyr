%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"
#include "parser.h"
#include "ops.h"
#include "parser_global.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s, ...);

static struct cstr *resizestr(struct cstr *s, size_t len);
static int savestr(const char *text, size_t len, yyscan_t yyscanner);
#define savechr(ch,sc) savestr((const char[]){ ch },1,sc)
static int translate_escape(YYLTYPE *loc, struct parse_data *extra, int what);

#define savecol  (yyextra->lexstate.savecol)
#define saveline (yyextra->lexstate.savep[yyextra->lexstate.swap])
#define SWAPLINE (yyextra->lexstate.swap ^= 1)

#define YY_USER_ACTION \
    do { \
        int len = MIN(yyleng, LINE_LEN - savecol); \
        strcopy(&saveline[savecol], yytext, len + 1); \
        saveline[savecol += len] = 0; \
        yylloc->first_line = yylloc->last_line = yylineno; \
        yylloc->first_column = yycolumn; \
        yylloc->last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng; \
    } while (0);

#define YSTR        yylval->cstr
#define PUSH(State) yy_push_state(State, yyscanner)
#define DUP         yy_push_state(yy_top_state(yyscanner), yyscanner)
#define POP         yy_pop_state(yyscanner)
#define NEWSTR      YSTR = resizestr(NULL, yyleng)
#define CHKSTR      do { if (YSTR->size < 0) { NEWSTR; } } while (0)
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
self    [][|&+*^><.,:@()~;/-]
strcon  ([^"]|\\\")
escchar [\\"'0nbrtfv]
white   [\t\f\v\r ]
edge    {white}|\n

%%

\t                  { yycolumn += 7; /* 1 added in YY_USER_ACTION */ }
illegal             { return ILLEGAL; }

{regname}           { yylval->i = yytext[0]; return REGISTER; }
{symbol}            { NEWSTR; SAVESTR; return SYMBOL; }
{local}             { NEWSTR; SAVESTR; return LOCAL; }

\"                  { NEWSTR; PUSH(string); return OPNQUOTE; }
<string>{
    \"              { POP; return CLSQUOTE; }
    \\{escchar}     { CHKSTR; savechr(translate_escape(yylloc, yyextra, yytext[1]), yyscanner); YSTR->size = -1; return STRSPAN; }
    [^\\"]+         { CHKSTR; SAVESTR; YSTR->size = -1; return STRSPAN; }
}

".word"{edge}       { PUSH(ignore_nl); return WORD; }
".utf32"{edge}      { PUSH(ignore_nl); return UTF32; }
".global"{edge}     { PUSH(ignore_nl); return GLOBAL; }
".set"{edge}        { PUSH(ignore_nl); return SET; }
".zero"{edge}       { PUSH(ignore_nl); return ZERO; }
".option"{edge}     { PUSH(ignore_nl); return OPTION; }

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
    <<EOF>>         { tenyr_error(yylloc, yyextra, "End of file reached while inside comment. "
                        "Did you forget `*/' ?"); POP; }
    .               ; /* ignored content */
}

"'"                 { PUSH(charend); PUSH(charlit); NEWSTR; }
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

<*>\n               { SWAPLINE; saveline[savecol = 0] = 0; yycolumn = 0; return '\n'; }

<*>.                tenyr_error(yylloc, yyextra, "unexpected character `%c`", yytext[0]);
                    /* LCOV_EXCL_START */
%%
/* LCOV_EXCL_STOP LCOV_EXCL_LINE */
/* LCOV_EXCL_LINE
 * XXXREMOVEXXX
 * a line above this one removed to work around a gcov off-by-one-line bug
 */
static struct cstr *resizestr(struct cstr *s, size_t len)
{
    if (!s) {
        // allocate at least a minimum segment size
        if (len < 128)
            len = 128;

        s = malloc(sizeof *s);
        s->size = len;
        s->head = s->tail = calloc(s->size, 1);
        s->last = s;
        s->right = NULL;
        return s;
    } else if (s->tail - s->head + len > s->size) {
        s->size = s->tail - s->head;        // shorten this string segment
        s->right = resizestr(NULL, len);    // allocate a new segment
        s->last = s->right;
        return s->right;                    // caller keeps original s
    }

    return s;
}

static int savestr(const char *text, size_t len, yyscan_t yyscanner)
{
    // savestr() appends a string piece into a buffer
    YYSTYPE *lval  = yyget_lval(yyscanner);
    struct cstr *s = lval->cstr;

    s = resizestr(s, len);
    memcpy(s->tail, text, len);
    s->tail += len;

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

