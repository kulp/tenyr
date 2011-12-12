%{
#include <stdio.h>
#include "parser.h"
#include "parser.h"
#include "ops.h"

struct parse_data;
int tenor_error(YYLTYPE *locp, struct parse_data *pd, const char *s);

static int savestr(yyscan_t yyscanner);
%}

%option reentrant bison-bridge bison-locations
%option noyywrap never-interactive yylineno batch
%option case-insensitive
%option extra-type="struct parse_data *"
%option prefix="tenor_"

regname ([A-P])
label   ([A-Z_][A-Z0-9_]{1,30})

%%

[\n;]                   ;
"illegal"               { return ILLEGAL; }

{regname}               { yylval->chr = yytext[0]; return REGISTER; }
{label}                 { savestr(yyscanner); return LABEL; }

[][|&+*%^>$.:@-]        { return yytext[0]; }

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

"//".*$                 ; // c99-style comments only

.                       { tenor_error(yylloc, yyextra, "unexpected character"); }

%%

static int savestr(yyscan_t yyscanner)
{
    int len          = yyget_leng(yyscanner);
    YYSTYPE *lval    = yyget_lval(yyscanner);
    YYLTYPE *lloc    = yyget_lloc(yyscanner);
    const char *text = yyget_text(yyscanner);
    void *extra      = yyget_extra(yyscanner);

    if (len >= (signed)sizeof lval->str) {
        tenor_error(lloc, extra, "token too long");
        return 1;
    } else {
        snprintf(lval->str, len + 1, "%s", text);
        return 0;
    }
}

