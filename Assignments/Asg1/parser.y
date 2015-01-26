%{

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#define YYPRINT yyprint

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%token TOK_EXIT

%start  program

%%

program  : TOK_EXIT {$$ = "exit found";} 

%%