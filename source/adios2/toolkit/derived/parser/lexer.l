%{   
#include "parser.hpp"
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;
%}

%option noyywrap


DIGIT    [0-9]
CHAR     ([a-z]|[A-Z])
   
%%

"+"            {yylval.sval=strdup(yytext); return OPERATOR;}
"-"            {yylval.sval=strdup(yytext); return OPERATOR;}
"*"            {yylval.sval=strdup(yytext); return OPERATOR;}
"/"            {yylval.sval=strdup(yytext); return OPERATOR;}
"^"            {yylval.sval=strdup(yytext); return OPERATOR;}
"("            {return L_PAREN;}
")"            {return R_PAREN;}
","            {return COMMA;}
"add"          {yylval.sval=strdup(yytext); return FUNCTION;}
"sqrt"         {yylval.sval=strdup(yytext); return FUNCTION;}
"sin"          {yylval.sval=strdup(yytext); return FUNCTION;}
"cos"          {yylval.sval=strdup(yytext); return FUNCTION;}
"tan"          {yylval.sval=strdup(yytext); return FUNCTION;}
"magnitude"    {yylval.sval=strdup(yytext); return FUNCTION;}
"curl"         {yylval.sval=strdup(yytext); return FUNCTION;}


(\.{DIGIT}+)|({DIGIT}+(\.{DIGIT}*)?([eE][+-]?[0-9]+)?)   {yylval.dval = atof(yytext); return NUMBER;}

[ \t]+         {/* ignore spaces */}

(\n|\0|EOF|$end)           {return ENDL;}

({CHAR}|{DIGIT}|_)+        {yylval.sval=strdup(yytext); return ALIAS;}

:(\\|\/|_|{DIGIT})*{CHAR}+(\\|\/|-|_|{DIGIT}|{CHAR})*      {yylval.sval=strndup(yytext + 1,strlen(yytext)-1); return PATH;}

'(\\|\/|_|{DIGIT})*{CHAR}+(\\|\/|-|_|{DIGIT}|{CHAR})*'     {yylval.sval=strndup(yytext + 1,strlen(yytext)-2); return PATH;}

"["({DIGIT}+|{DIGIT}*":"{DIGIT}*":"{DIGIT}*)(,({DIGIT}+|{DIGIT}*":"{DIGIT}*":"{DIGIT}*))*"]"                            {yylval.sval=strndup(yytext + 1,strlen(yytext)-2); return INDICES;}

.              {printf("Error at line %d: unrecognized symbol \"%s\"\n", yylloc.first_line, yytext); exit(0);}

%%