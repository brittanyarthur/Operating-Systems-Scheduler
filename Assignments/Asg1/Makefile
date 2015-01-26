all: flex compile

bison:
	bison --defines=yyparse.h --output=yyparse.cpp parser.y

flex:
	flex -oyylex.c scanner.l

compile:
	cc myshell.c yylex.c -lfl