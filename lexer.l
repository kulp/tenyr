%option noyywrap

%{
    #include <stdio.h>
    #include "parser.h"
    #include "ops.h"
    int lineno;
    int column; // TODO
    int yyerror(const char *msg);
    static void c(void);
    static void skip_comment(int style);
    static int savestr(void);
    enum { C89_STYLE, C99_STYLE };
%}

regname ([A-Pa-p])
label   ([A-Za-z_][A-Za-z0-9_]{1,30})

%%

"\n"                    { c(); lineno++; }
"illegal"               { c(); return ILLEGAL; }

{regname}               { c(); yylval.chr = yytext[0]; return REGISTER; }
{label}                 { c(); savestr(); return LABEL; }

"["                     { c(); return '[';  }
"]"                     { c(); return ']';  }
"|"                     { c(); return '|';  }
"&"                     { c(); return '&';  }
"+"                     { c(); return '+';  }
"*"                     { c(); return '*';  }
"%"                     { c(); return '%';  }
"<<"                    { c(); return LSH;  }
"<="                    { c(); return LTE;  }
"=="                    { c(); return EQ;   }
"~|"                    { c(); return NOR;  }
"~&"                    { c(); return NAND; }
"^"                     { c(); return '^';  }
"-"                     { c(); return '-';  }
"^~"                    { c(); return XORN; }
">>"                    { c(); return RSH;  }
">"                     { c(); return '>';  }
"<>"                    { c(); return NEQ;  }
"<-"                    { c(); return TOL;  }
"->"                    { c(); return TOR;  }
"$"                     { c(); return '$';  }
"."                     { c(); return '.';  }
":"                     { c(); return ':';  }
"@"                     { c(); return '@';  }

-?"0"[xX][0-9a-fA-F]+   { c(); savestr(); return INTEGER; }
-?"0"[0-7]+             { c(); savestr(); return INTEGER; }
-?[0-9]+                { c(); savestr(); return INTEGER; }

[\t\f\v\r ]+            { c(); }

"/*"                    { skip_comment(C89_STYLE); }
"//"                    { skip_comment(C99_STYLE); }

.                       { c(); yyerror("unexpected character"); }

%%

static void c()
{
    for (int i = 0; yytext[i]; i++)
        if (yytext[i] == '\n')
            column = 0;
        else
            column++;
}

static int savestr(void)
{
    if ((signed)yyleng >= (signed)sizeof yylval.str) {
        yyerror("label too long");
        return 1;
    } else {
        snprintf(yylval.str, yyleng + 1, "%s", yytext);
        return 0;
    }
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
                ++lineno, column = 0;
            c1 = c2;
            c2 = input();
        }
    } else {
        while (c2 != EOF && !(c1 == '\n'))
            c1 = input();
        if (c1 == '\n') 
            ++lineno, column = 0;
    }
}

void switch_to_stream(FILE *f)
{
    yyin = f;
    yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));
    BEGIN(INITIAL);
}

