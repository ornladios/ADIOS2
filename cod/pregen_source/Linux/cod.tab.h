/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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

#ifndef YY_YY_USERS_EISEN_PROG_KORVO_BUILD_BUILD_AREA_FFS_DARWIN10_13_COD_TAB_H_INCLUDED
# define YY_YY_USERS_EISEN_PROG_KORVO_BUILD_BUILD_AREA_FFS_DARWIN10_13_COD_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ARROW = 258,
    LPAREN = 259,
    RPAREN = 260,
    LCURLY = 261,
    RCURLY = 262,
    COLON = 263,
    QUESTION = 264,
    LBRACKET = 265,
    RBRACKET = 266,
    DOT = 267,
    STAR = 268,
    AT = 269,
    SLASH = 270,
    MODULUS = 271,
    PLUS = 272,
    MINUS = 273,
    TILDE = 274,
    LEQ = 275,
    LT = 276,
    GEQ = 277,
    GT = 278,
    EQ = 279,
    NEQ = 280,
    LEFT_SHIFT = 281,
    RIGHT_SHIFT = 282,
    ASSIGN = 283,
    MUL_ASSIGN = 284,
    DIV_ASSIGN = 285,
    MOD_ASSIGN = 286,
    ADD_ASSIGN = 287,
    SUB_ASSIGN = 288,
    LEFT_ASSIGN = 289,
    RIGHT_ASSIGN = 290,
    AND_ASSIGN = 291,
    XOR_ASSIGN = 292,
    OR_ASSIGN = 293,
    LOG_OR = 294,
    LOG_AND = 295,
    ARITH_OR = 296,
    ARITH_AND = 297,
    ARITH_XOR = 298,
    INC_OP = 299,
    DEC_OP = 300,
    BANG = 301,
    SEMI = 302,
    IF = 303,
    ELSE = 304,
    FOR = 305,
    DO = 306,
    WHILE = 307,
    CHAR = 308,
    SHORT = 309,
    INT = 310,
    LONG = 311,
    UNSIGNED = 312,
    SIGNED = 313,
    FLOAT = 314,
    DOUBLE = 315,
    VOID = 316,
    STRING = 317,
    STATIC = 318,
    EXTERN_TOKEN = 319,
    STRUCT = 320,
    ENUM = 321,
    UNION = 322,
    CONST = 323,
    SIZEOF = 324,
    TYPEDEF = 325,
    RETURN_TOKEN = 326,
    CONTINUE = 327,
    BREAK = 328,
    GOTO = 329,
    PRINT = 330,
    COMMA = 331,
    DOTDOTDOT = 332,
    integer_constant = 333,
    character_constant = 334,
    string_constant = 335,
    floating_constant = 336,
    identifier_ref = 337,
    type_id = 338,
    enumeration_constant = 339
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 183 "cod/cod.y" /* yacc.c:1909  */

    lx_info info;
    sm_ref reference;
    operator_t operator;
    sm_list list;
    char *string;

#line 147 "/Users/eisen/prog/korvo_build/build_area/ffs/darwin10.13/cod.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_USERS_EISEN_PROG_KORVO_BUILD_BUILD_AREA_FFS_DARWIN10_13_COD_TAB_H_INCLUDED  */
