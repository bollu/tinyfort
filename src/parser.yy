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
  std::vector<tf::Expr *>*exprtuple;
  std::vector<pair<std::string, tf::Type*>>*fnparams;
  tf::Stmt *stmt;
  tf::Expr *expr;
  tf::LVal *lval;
  std::string *s;
  tf::FnDefn *fndefn;
  tf::Type *type;
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
%token LET
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
%token IF;
%token ELSE;
%token FOR;
%token WHILE;
%token OPENSQUARE;
%token CLOSESQUARE;
%token INT;
%token FLOAT;
%token BOOL;

%start toplevel
%type <program> program
%type <block>	block;
%type <stmt>	stmt;
%type <lval>	lval;
%type <expr>	expr;
%type <expr>	expr2;
%type <expr>	expr3;
%type <expr>	expr4;
%type <exprtuple> exprtuple_;
%type <exprtuple> exprtuple;
%type <stmts>	stmts;
%type <UNDEF> topleveldefn;
%type <fndefn> fndefn;
%type <fnparams> fnparams;
%type <fnparams> fnparamsNonEmpty;
%type <typebase> typebase;
%type <type>	basetype;
%type <type>	type;

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
     | expr3 PLUS expr2 { $$ = new tf::ExprBinop($1, tf::Binop::BinopAdd, $3); }
     | expr3 MINUS expr2 { $$ = new tf::ExprBinop($1, tf::Binop::BinopSub, $3); }

// * , /
expr3: 
  expr4 STAR expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopMul, $3); }
  | expr4 DIVIDE expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopDiv, $3); }
  | expr4 { $$ = $1; }



// root literals
expr4: 
     OPENPAREN expr CLOSEPAREN { $$ = $2; }
     |  INTEGER { $$ = new tf::ExprInt($1); } 
     | lval { $$ = new tf::ExprLVal($1); }

exprtuple_: 
  exprtuple_ COMMA expr  { $$ = $1; $$->push_back($3); }
  | expr { $$ = new std::vector<tf::Expr *>(); $$->push_back($1); }


exprtuple: OPENSQUARE CLOSESQUARE { $$ = new std::vector<tf::Expr *>(); } 
         | OPENSQUARE exprtuple_ CLOSESQUARE { $$ = $2; }

lval : IDENTIFIER {
    $$ = new tf::LValIdent(*$1);
 } |
 IDENTIFIER exprtuple {
   $$ = new tf::LValArray(*$1, *$2);
 }

type : basetype {
     $$ = $1;
 } | basetype exprtuple {
     $$ = new tf::TypeArray(((tf::TypeBase*)$1)->t, *$2);
 }

basetype: INT {
        $$ = new tf::TypeBase(tf::TypeBaseName::Int);
        }
    | FLOAT {
        $$ = new tf::TypeBase(tf::TypeBaseName::Float);
    }

stmt : lval EQUALS expr SEMICOLON {
         $$ = new tf::StmtSet($1, $3);
     }
    | WHILE expr block {
         $$ = new tf::StmtWhileLoop($2, $3);
     }
    | IDENTIFIER COLON type SEMICOLON {
       $$ = new tf::StmtLet(*$1, $3);
    }

stmts: stmts stmt {
         $$ = $1;
         $$->push_back($2);
     }
     | stmt {
       $$ = new std::vector<tf::Stmt*>();
       $$->push_back($1);
     }
     

fnparams: OPENPAREN CLOSEPAREN {
        $$ = new vector<pair<string, tf::Type*>>();
} | OPENPAREN fnparamsNonEmpty CLOSEPAREN {
  $$ = $2;
}

fnparamsNonEmpty: 
 IDENTIFIER COLON type { 
                $$ = new vector<pair<string, tf::Type*>>();
                $$->push_back({*$1, $3});
 } | fnparamsNonEmpty COMMA IDENTIFIER COLON type {
   $$ = $1;
   $$->push_back({*$3, $5});
 }

fndefn: DEF IDENTIFIER fnparams block {
      $$ = new tf::FnDefn(*$2, *$3, $4);
  }
%%

