/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEQ = 258,
     EQ = 259,
     LTE = 260,
     XORN = 261,
     NOR = 262,
     NAND = 263,
     RSH = 264,
     LSH = 265,
     TOL = 266,
     TOR = 267,
     INTEGER = 268,
     LABEL = 269,
     CSTRING = 270,
     REGISTER = 271,
     ILLEGAL = 272,
     WORD = 273,
     ASCII = 274,
     ASCIZ = 275
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 76 "src/parser.y"

    int32_t i;
    signed s;
    struct const_expr {
        enum { OP2, LAB, IMM, ICI } type;
        int32_t i;
        char labelname[32]; // TODO document length
        int op;
        struct instruction *insn; // for '.'-resolving
        struct const_expr *left, *right;
    } *ce;
    struct expr {
        int deref;
        int x;
        int op;
        int y;
        int32_t i;
        int width;  ///< width of relocation XXX cleanup
        int mult;   ///< multiplier from addsub
        struct const_expr *ce;
    } *expr;
    struct cstr {
        int len;
        char *str;
        struct cstr *right;
    } *cstr;
    struct instruction *insn;
    struct instruction_list *program;
    char str[64]; // TODO document length
    char chr;
    int op;
    int arrow;



/* Line 2068 of yacc.c  */
#line 106 "src/gen/parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



