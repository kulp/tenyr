%{
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "parser.h"
#include "ops.h"
#include "parser_global.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s);

static int savestr(yyscan_t yyscanner);
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

%}

%option reentrant bison-bridge bison-locations
%option noyywrap never-interactive yylineno batch
%option case-insensitive
%option extra-type="struct parse_data *"
%option prefix="tenyr_"

%x charlit charend charesc

regname ([A-P])
/* don't permit collision with regname */
symbol   ([A-Z_][A-Z0-9_]{1,30}|[Q-Z_][A-Z0-9_]{0,30})
/* local symbols start with ".L" */
local   (\.L[A-Z0-9_]{1,31})
string  (([^"]|\\\")+)
hexnum  ("0"[xX][0-9a-fA-F]+)
octnum  ("0"[0-7]+)
binnum  ("0"[bB][01]+)
decnum  ([0-9]+)
regnum  {hexnum}|{octnum}|{decnum}

%%

";"                     ;
"\t"                    { yycolumn += 7; /* 1 added in YY_USER_ACTION */ }
"\n"                    { saveline[savecol = 0] = 0; }
"illegal"               { return ILLEGAL; }

{regname}               { yylval->chr = yytext[0]; return REGISTER; }
{symbol}                { savestr(yyscanner); return SYMBOL; }
{local}                 { savestr(yyscanner); return LOCAL; }
"\""{string}"\""        { savestr(yyscanner); return STRING; }

".word"                 { return WORD; }
".ascii"                { return ASCII; }
".utf32"                { return UTF32; }
".global"               { return GLOBAL; }
".set"                  { return SET; }

[][|&+*^<>.,:@()$~-]    { return yytext[0]; }

"<<"                    { return LSH;  }
"=="                    { return EQ;   }
"~&"                    { return NAND; }
"<>"                    { return NEQ;  }
"^~"                    { return XORN; }
">>"                    { return RSH;  }
"<-"                    { return TOL;  }
"->"                    { return TOR;  }

{regnum}                { yylval->i = strtoll(&yytext[0], NULL, 0); return INTEGER; }
{binnum}                { yylval->i = strtoll(&yytext[2], NULL, 2); return INTEGER; }

[\t\f\v\r ]+            ;

"//".*$                 ; // c99-style comments
"#".*$                  ; // bash-style comments

"'"                     BEGIN charlit;
<charend>"'"            BEGIN INITIAL;
<charlit>\\             BEGIN charesc;
<charlit>.              { yylval->i = yytext[0]; BEGIN charend; return INTEGER; }
<charesc>[nbrtfv\\]     { yylval->i = translate_escape(yytext[0]); BEGIN charend; return INTEGER; }

.                       { tenyr_error(yylloc, yyextra, "unexpected character"); }

%%

static int savestr(yyscan_t yyscanner)
{
    int len          = yyget_leng(yyscanner);
    YYSTYPE *lval    = yyget_lval(yyscanner);
    YYLTYPE *lloc    = yyget_lloc(yyscanner);
    const char *text = yyget_text(yyscanner);
    void *extra      = yyget_extra(yyscanner);

    if (len >= (signed)sizeof lval->str) {
        tenyr_error(lloc, extra, "token too long");
        return 1;
    } else {
        memset(lval->str, 0, sizeof lval->str);
        strcopy(lval->str, text, len + 1);
        return 0;
    }
}

static int translate_escape(int what)
{
    switch (what) {
        case '\\': return '\\';
        case 'b' : return '\b';
        case 'f' : return '\f';
        case 'n' : return '\n';
        case 'r' : return '\r';
        case 't' : return '\t';
        case 'v' : return '\v';
        default: return -1;
    }
}

