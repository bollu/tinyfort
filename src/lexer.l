%{
#pragma clang diagnostic ignored "-Wdeprecated-register"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#include <math.h>
#include <iostream>
#include <assert.h>
#include <string>
#include <stdio.h>
#include "tinyfort/ir.h"
#include <cstdio>

// Keep this at the end so it can see all the includes made here.
#include "parser.generated.hpp"


using namespace std;
//extern "C" int yylex();
//extern "C" int yyparse();


int yylex();
int yyparse();

extern tf::Program *g_program;
int g_lexer_line_num = 1;
int g_lexer_success = 1;

int compile_program(int argc, char **argv, tf::Program *p);
%}


%%
['].*['] { yylval.str = new std::string(yytext); return CHAR; }
["].*?["] { yylval.str = new std::string(yytext); return STRING; }
"("            return OPENPAREN; 
")"            return CLOSEPAREN;
"["            return OPENSQUARE; 
"]"            return CLOSESQUARE;
"{"            return OPENFLOWER;
"}"            return CLOSEFLOWER;
";"            return SEMICOLON;
","            return COMMA;
":"            return COLON;
"->"           return THINARROW;
"="            return EQUALS;
"let"          return LET;
"def"          return DEF;
"+"          return PLUS;
"-"          return MINUS;
"*"          return STAR;
"/"          return DIVIDE;
"<"          return LT;
"<="          return LEQ;
"=="          return CMPEQ;
"!="          return CMPNEQ;
">="          return GEQ;
"while"          return WHILE;
"for"          return FOR;
"if"          return IF;
"elif"          return ELIF;
"else"          return ELSE;
"int"          return INTTY;
"char"         return CHARTY;
"FILE"          return FILETY;
"float"          return FLOATTY;
"bool"          return BOOLTY;
"void"          return VOIDTY;
"return"          return RETURN;
"import"         return IMPORT;

[ \t]          ;

[0-9]+           { 

                     yylval.i = atoi(yytext);
                     return INTEGER;
                 }

[A-Za-z][a-zA-Z0-9_]*     {
                    yylval.str = new std::string(yytext);
                    return IDENTIFIER;
                 }

#.*            { /* DO NOTHING */ }
\n             { ++g_lexer_line_num; }
%%

int yywrap(void) {
    return 1;
}

int main(int argc, char **argv) {
    assert(argc >= 2);
    std::string OPTION_INPUT_FILENAME = std::string(argv[1]);

    yyin = fopen(OPTION_INPUT_FILENAME.c_str(), "r");
    if (yyin == NULL) {
        printf("unable to open input file: %s\n", OPTION_INPUT_FILENAME.c_str());
        perror("file open error: ");
        return 1;
    }

    do {
        yyparse();
    } while (!feof(yyin));
    if (g_lexer_success) {
        return compile_program(argc, argv, g_program);
    } else {
        return 1;
    }
}
