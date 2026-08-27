/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

%skeleton "lalr1.cc"
%require "3.8.2"
%header

%define api.token.raw
%define api.namespace {adios2::detail}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <tuple>
  #include <vector>
  #include <string>
  namespace adios2
  {
    namespace detail
    {
      class ASTDriver;
    }
  }
}

// The parsing context.
%param { ASTDriver& drv }

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%code {
#include "ASTDriver.h"
#include "ASTNode.h"
#include <sstream>
#include <string>
}

%define api.token.prefix {TOK_}
%token
  ASSIGN  "="
  COMMA   ","
  L_PAREN "("
  R_PAREN ")"
  PLUS    "+"
  MINUS   "-"
  STAR    "*"
  SLASH   "/"
  CARET   "^"
;

%token <std::string> IDENTIFIER "identifier"
%token <std::string> VARNAME
%token <std::string> ATTRNAME
%token <double> NUM
%nterm <int> list

%left PLUS MINUS
%left STAR SLASH
%precedence UMINUS
%right CARET

%%
%start lines;
lines:
  assignment lines {}
| exp {}
;

assignment:
  IDENTIFIER ASSIGN VARNAME { drv.add_lookup_entry($1,  $3); }
| IDENTIFIER ASSIGN IDENTIFIER { drv.add_lookup_entry($1,  $3); }
| IDENTIFIER ASSIGN NUM { drv.add_constant_entry($1, $3); }
| IDENTIFIER ASSIGN ATTRNAME { drv.add_attribute_entry($1, $3); }
;

exp:
  NUM { drv.add_number($1); }
| exp "+" exp   { drv.createNode("ADD", 2); }
| exp "-" exp   { drv.createNode("SUBTRACT", 2); }
| exp "*" exp   { drv.createNode("MULT", 2); }
| exp "/" exp   { drv.createNode("DIV", 2); }
| exp "^" exp   { drv.createNode("POW", 2); }
| "-" exp %prec UMINUS  { drv.createNode("NEGATE", 1); }
| "(" exp ")" {  }
| IDENTIFIER "(" list ")" { drv.createNode($1, $3); }
| IDENTIFIER  { drv.createNode($1); }
| VARNAME  { drv.createNode($1); }
| ATTRNAME  { drv.add_attribute_node($1); }
;


list:
  %empty { $$ = 0; }
| exp COMMA list { $$ = $3 + 1; }
| exp { $$ = 1; }
%%

void
adios2::detail::parser::error (const location_type& l, const std::string& m)
{
  std::ostringstream os;
  os << l << ": " << m;
  drv.hasError = true;
  drv.errorMessage = os.str();
}
