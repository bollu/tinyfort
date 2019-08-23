%{
#include <math.h>
#include <stack>
#include <iostream>
#include <vector>
#include "tinyfor/stgir.h"

using namespace std;

#define YYERROR_VERBOSE
using namespace std;
using namespace stg; 
using namespace llvm;
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
std::vector<FnDefn *> g_fndefns;


%}

// %union{
//   std::vector<Atom *> *atomslist;
//   std::vector<TypeName> *typeslist;
//   std::vector<stg::CaseAlt*> *altslist;
//   stg::Atom *atom;
//   stg::CaseAlt *alt;
//   stg::Expression *expr;
//   stg::Lambda *lambda;
//   stg::Binding *binding;
//   stg::DataType *datatype;
//   stg::Parameter *param;
//   stg::DataConstructor *dataconstructor;
//   std::string *constructorName;
//   stg::TypeRaw *typeraw;
// 
//   bool UNDEF;
// }

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
%token BINDING
%token DATA
%token LET
%token IN
%token DEFAULT

%start toplevel
%token <atom>	ATOMINT
%token <atom>	ATOMSTRING

%type <program> program

%%
toplevel:
        program {
                  g_program = new tf::Program();
              }
program:
  program topleveldefn
  | topleveldefn

topleveldefn:
  fndefn { g_fndefns.push_back($1); }

atom: 
  ATOMINT | ATOMSTRING

atoms_: 
  atoms_ atom {
    $$ = $1;
    $$->push_back($2);
  }
  | atom {  $$ = new std::vector<Atom*>(); $$->push_back($1); }

atomlist: OPENPAREN atoms_ CLOSEPAREN {
  $$ = $2;
}
| OPENPAREN CLOSEPAREN {
    $$ = new std::vector<Atom*>();
}
%%

