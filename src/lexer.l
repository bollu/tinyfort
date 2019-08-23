%{
#pragma clang diagnostic ignored "-Wdeprecated-register"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#include <math.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include "tinyfort/ir.h"
#include "llvm/Support/CommandLine.h"
#include <cstdio>

static const char *VERSION_STRING = "0.1.0";

using namespace stg;

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

int compile_program(stg::Program *p, cxxopts::Options &options);

llvm::cl::opt<std::string> OPTION_INPUT_FILENAME(llvm::cl::Positional, llvm::cl::desc("<input filename>"), llvm::cl::Required);
%}


%%
"="            return ASSIGN; 
"("            return OPENPAREN; 
")"            return CLOSEPAREN;
"{"            return OPENFLOWER;
"}"            return CLOSEFLOWER;
";"            return SEMICOLON;
","            return COMMA;
":"            return COLON;
"->"           return THINARROW;
"set"          return SET;

[ \t]          ;

[0-9]+           { 
                    yylval.atom= new AtomInt(atoi(yytext));
                    return ATOMINT;
                 }
[A-Za-z][a-zA-Z0-9_]*     {
                    yylval.atom = new AtomIdent(yytext);
                    return ATOMSTRING;
                 }

#.*            { /* DO NOTHING */ }
\n             { ++g_lexer_line_num; }
%%

int yywrap(void) {
    return 1;
}

void printHelp(cxxopts::Options &options) {
     if (options.count("help"))
     {
          std::cout << options.help({"", "Group"}) << std::endl;
          exit(0);
    }
}

int main(int argc, char **argv) {
    assert(argv >= 2);
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
        return compile_program(g_program, options);
    } else {
        return 1;
    }
}
