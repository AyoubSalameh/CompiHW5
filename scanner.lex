%{
/* Declaration section*/
#include <stdio.h>
#include "parser.h"
#include "hw3_output.hpp"
#include "parser.tab.hpp"
%}

%option yylineno
%option noyywrap


plus_minus  \+|\-
mul_div \*|\/
comment     \/\/[^\r\n]*[ \r|\n|\r\n]?
whitespace [\t\n\r ]
tab [\t]



%%
void                            {yylval = new Node(yytext); return VOID;}
int                             {yylval = new Node(yytext); return INT;}
byte                            {yylval = new Node(yytext); return BYTE;}
b                               {yylval = new Node(yytext); return B;}
bool                            {yylval = new Node(yytext); return BOOL;}
override                        {yylval = new Node(yytext); return OVERRIDE;}
and                             {yylval = new Node(yytext); return AND;}
or                              {yylval = new Node(yytext); return OR;}
not                             {yylval = new Node(yytext); return NOT;}
true                            {yylval = new Node(yytext); return TRUE;}
false                           {yylval = new Node(yytext); return FALSE;}
return                          {yylval = new Node(yytext); return RETURN;}
if                              {yylval = new Node(yytext); return IF;}
else                            {yylval = new Node(yytext); return ELSE;}
while                           {yylval = new Node(yytext); return WHILE;}
break                           {yylval = new Node(yytext); return BREAK;}
continue                        {yylval = new Node(yytext); return CONTINUE;}
;                               {yylval = new Node(yytext); return SC;}
,                               {yylval = new Node(yytext); return COMMA;}
\(                              {yylval = new Node(yytext); return LPAREN;}
\)                              {yylval = new Node(yytext); return RPAREN;}
\{                              {yylval = new Node(yytext); return LBRACE;}
\}                              {yylval = new Node(yytext); return RBRACE;}
=                               {yylval = new Node(yytext); return ASSIGN;}
==|!=                           {yylval = new Node(yytext); return EQUALITY;}
\<=|\>=|\<|\>                   {yylval = new Node(yytext); return RELATION;}
{plus_minus}                    {yylval = new Node(yytext); return PLUS_MINUS;}
{mul_div}                       {yylval = new Node(yytext); return MUL_DIV;}
[a-zA-Z][a-zA-Z0-9]*            {yylval = new Node(yytext); return ID;}
0|[1-9][0-9]*                   {yylval = new Node(yytext); return NUM;}
\"([^\n\r\"\\]|\\[rnt"\\])+\"   {yylval = new Node(yytext); return STRING;}
{whitespace}                    ;
{comment}                       ;
.                               { output::errorLex(yylineno);
                                exit(0);}
%%


