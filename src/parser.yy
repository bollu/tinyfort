%{
#include <math.h>
#include <stack>
#include <iostream>
#include <vector>
#include "tinyfort/ir.h"
using namespace std;

#define YYERROR_VERBOSE
using namespace std;
//extern "C" int yylex(void);
//extern "C" int yyparse(void);
int yylex(void);
int yyparse(void);

extern int g_lexer_success;
extern int g_lexer_line_num;

void yyerror(const char *s) {
  fprintf(stderr, "line %d: %s\n", g_lexer_line_num, s);
  g_lexer_success = 0;
}


tf::Program *g_program;
std::vector<tf::FnDefn *> g_fndefns;


%}

%union{
  std::string *s;
  tf::FnDefn *fndefn;
  int i;
}

%token ASSIGN
%token OPENPAREN
%token COMMA
%token CLOSEPAREN
%token CASE
%token OF
%token SEMICOLON
%token COLON
%token THINARROW
%token PIPE
%token EOFTOKEN
%token LAMBDA
%token OPENFLOWER
%token CLOSEFLOWER
%token DEF
%token BINDING
%token SET

%start toplevel
%type <program> program
%type <UNDEF> topleveldefn;
%type <fndefn> fndefn
%token <i>	INTEGER;
%token <s>	IDENTIFIER;
%%
toplevel:
        program {
                  g_program = new tf::Program(g_fndefns);
              }
program:
  program topleveldefn
  | topleveldefn

topleveldefn:
  fndefn { g_fndefns.push_back($1); }

fndefn: DEF IDENTIFIER OPENPAREN CLOSEPAREN {
      $$ = new tf::FnDefn(*$2);
      }
%%

