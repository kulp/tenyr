%option noyywrap

%{
    #include "parser.h"
    #include "ops.h"
    int lineno;
    int column; // TODO
    static void c(void);
    static void skip_comment(int style);
    enum { C89_STYLE, C99_STYLE };
%}

regname ([A-Pa-p])

%%

"\n"                { c(); lineno++; }
{regname}           { c(); yylval.chr = yytext[0]; return REGISTER; }
"["                 { c(); return '[';  }
"]"                 { c(); return ']';  }
"|"                 { c(); return '|';  }
"&"                 { c(); return '&';  }
"+"                 { c(); return '+';  }
"*"                 { c(); return '*';  }
"%"                 { c(); return '%';  }
"<<"                { c(); return LSH;  }
"<="                { c(); return LTE;  }
"=="                { c(); return EQ;   }
"~|"                { c(); return NOR;  }
"~&"                { c(); return NAND; }
"^"                 { c(); return '^';  }
"-"                 { c(); return '-';  }
"^~"                { c(); return XORN; }
">>"                { c(); return RSH;  }
">"                 { c(); return '>';  }
"<>"                { c(); return NEQ;  }
"<-"                { c(); return TOL;  }
"->"                { c(); return TOR;  }

"0"[xX][0-9a-fA-F]+ { c(); yylval.str = yytext; return INTEGER; }
"0"[0-7]+           { c(); yylval.str = yytext; return INTEGER; }
[0-9]+              { c(); yylval.str = yytext; return INTEGER; }

[\t\f\v\r ]+        { c(); }

"/*"                { skip_comment(C89_STYLE); }
"//"                { skip_comment(C99_STYLE); }

%%

static void c()
{
    for (int i = 0; yytext[i]; i++)
        if (yytext[i] == '\n')
            column = 0;
        else
            column++;
}


/*
 * We use this routine instead a lex pattern because we don't need
 * to save the matched comment in the `yytext' buffer.
 */
static void skip_comment(int style)
{
    int c1, c2 = 0;

    c1 = input();
    if (style == C89_STYLE) {
        c2 = input();

        while (c2 != EOF && !(c1 == '*' && c2 == '/')) {
            if (c1 == '\n')
                ++lineno;
            c1 = c2;
            c2 = input();
        }
    } else {
        while (c2 != EOF && !(c1 == '\n'))
            c1 = input();
        if (c1 == '\n')
            ++lineno;
    }
}

