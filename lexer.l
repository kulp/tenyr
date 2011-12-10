%option noyywrap

%{
    #include "parser.h"
    #include "ops.h"
    int lineno;
    int column; // TODO
    static void c();
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
"+-"                { c(); return SUB;  } 
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

%%

static void c()
{
    for (int i = 0; yytext[i]; i++)
        if (yytext[i] == '\n')
            column = 0;
        else
            column++;
}


