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
  tf::Block *block;
  std::vector<tf::Stmt *>*stmts;
  tf::Stmt *stmt;
  tf::Expr *expr;
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
%token EQUALS
%token PLUS
%token MINUS
%token STAR
%token DIVIDE
%token LT
%token GT
%token LEQ;
%token CMPEQ;
%token CMPNEQ;
%token GEQ;

%start toplevel
%type <program> program
%type <block>	block;
%type <stmt>	stmt;
%type <expr>	expr;
%type <expr>	expr2;
%type <expr>	expr3;
%type <expr>	expr4;
%type <stmts>	stmts;
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

block: OPENFLOWER CLOSEFLOWER { $$ = new tf::Block({}); }
     | OPENFLOWER stmts CLOSEFLOWER {
         $$ = new tf::Block(*$2);
     }

expr: 
     expr2 LEQ expr2 { $$ = new tf::ExprBinop($1, tf::Binop::BinopLeq, $3); }
     | expr2  { $$ = $1; }

// relational
expr2 : expr3 {
     $$ = $1;
     }
     | expr3 PLUS expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopAdd, $3); }
     | expr3 MINUS expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopSub, $3); }

// * , /
expr3: 
  expr4 STAR expr4 { $$ = new tf::ExprBinop($1, tf::Binop::BinopMul, $3); }
  | expr4 DIVIDE expr4 { $$ = new tf::ExprBinop($1, tf::Binop::BinopDiv, $3); }
  | expr4 { $$ = $1; }



// root literals
expr4 : INTEGER {
     $$ = new tf::ExprInt($1);
     } 


stmt : SET IDENTIFIER EQUALS expr SEMICOLON {
     $$ = new tf::StmtSet(*$2, $4);
     }
stmts: stmts stmt {
         $$ = $1;
         $$->push_back($2);
     }
     | stmt {
       $$ = new std::vector<tf::Stmt*>();
       $$->push_back($1);
     }
     

fndefn: DEF IDENTIFIER OPENPAREN CLOSEPAREN block {
      $$ = new tf::FnDefn(*$2, $5);
      }
%%

