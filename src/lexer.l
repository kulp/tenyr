%{
#include <stdio.h>
#include "parser.h"
#include "ops.h"
#include "common.h"
#include "parser_global.h"

int tenyr_error(YYLTYPE *locp, struct parse_data *pd, const char *s);

static int savestr(yyscan_t yyscanner);

#define savecol  yyextra->lexstate.savecol
#define saveline yyextra->lexstate.saveline

#define YY_USER_ACTION \
    do { \
        int len = MIN(yyleng, sizeof saveline - savecol); \
        strncpy(&saveline[savecol], yytext, len); \
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

regname ([A-P])
/* don't permit collision with regname */
label   ([A-Z_][A-Z0-9_]{1,30}|[Q-Z_][A-Z0-9_]{0,30})

%%

";"                     ;
"\t"                    { yycolumn += 7; /* 1 added in YY_USER_ACTION */ }
"\n"                    { saveline[savecol = 0] = 0; }
"illegal"               { return ILLEGAL; }

{regname}               { yylval->chr = yytext[0]; return REGISTER; }
{label}                 { savestr(yyscanner); return LABEL; }
".word"                 { return WORD; }

[][|&+*^>.:@()-]        { return yytext[0]; }

"<<"                    { return LSH;  }
"<="                    { return LTE;  }
"=="                    { return EQ;   }
"~|"                    { return NOR;  }
"~&"                    { return NAND; }
"^~"                    { return XORN; }
">>"                    { return RSH;  }
"<>"                    { return NEQ;  }
"<-"                    { return TOL;  }
"->"                    { return TOR;  }

-?"0"[xX][0-9a-fA-F]+   { savestr(yyscanner); return INTEGER; }
-?"0"[0-7]+             { savestr(yyscanner); return INTEGER; }
-?[0-9]+                { savestr(yyscanner); return INTEGER; }

[\t\f\v\r ]+            ;

"//".*$                 ; // c99-style comments
"#".*$                  ; // bash-style comments

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
        strncpy(lval->str, text, len + 1);
        return 0;
    }
}

