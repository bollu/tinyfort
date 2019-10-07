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
std::vector<tf::FnImport *> g_fnimports;
std::vector<tf::VarImport *> g_varimports;
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
  std::string *str;
  tf::FnImport *fnimport;
  tf::VarImport *varimport;
  tf::FnDefn *fndefn;
  tf::Type *type;
  int i;
  char c;
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
%token ELIF;
%token ELSE;
%token FOR;
%token WHILE;
%token OPENSQUARE;
%token CLOSESQUARE;
%token INTTY;
%token FLOATTY;
%token BOOLTY;
%token CHARTY;
%token VOIDTY;
%token RETURN;
%token FILETY;
%token IMPORT;

%start toplevel
%type <block>	block;
%type <stmt>	stmt;
%type <stmt>	iftail;
%type <lval>	lval;
%type <expr>	expr;
%type <expr>	expr2;
%type <expr>	expr3;
%type <expr>	expr4;
%type <exprtuple> exprtuple_;
%type <exprtuple> exprtuple;
%type <stmts>	stmts;
%type <UNDEF> topleveldefn;
%type <UNDEF> program;
%type <fndefn> fndefn;
%type <fnimport> fnimport;
%type <varimport> varimport;
%type <fnparams> fnparams;
%type <fnparams> fnparamsNonEmpty;
%type <type>	basetype;
%type <type>	type;

%token <i>	INTEGER;
%token <str>	IDENTIFIER;
%token <str>  STRING;
%token <c>  CHAR;
%%
toplevel:
        program { 
        g_program = new tf::Program(g_fnimports, g_varimports, g_fndefns);
              }
program:
  program topleveldefn
  | topleveldefn

topleveldefn:
  fndefn { g_fndefns.push_back($1); }
  | fnimport { g_fnimports.push_back($1); }
  | varimport { g_varimports.push_back($1); }

block: OPENFLOWER CLOSEFLOWER { $$ = new tf::Block({}); }
     | OPENFLOWER stmts CLOSEFLOWER {
         $$ = new tf::Block(*$2);
     }

expr: 
     expr2 LEQ expr2 { $$ = new tf::ExprBinop($1, tf::Binop::BinopLeq, $3); }
     | expr2 CMPEQ expr2 { $$ = new tf::ExprBinop($1, tf::Binop::BinopCmpEq, $3); }
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
     | STRING { $$ = new tf::ExprString(*$1); }
     | lval { $$ = new tf::ExprLVal($1); }
     | CHAR { $$ = new tf::ExprChar($1); }

exprtuple_: 
  exprtuple_ COMMA expr  { $$ = $1; $$->push_back($3); }
  | expr { $$ = new std::vector<tf::Expr *>(); $$->push_back($1); }


exprtuple: OPENSQUARE CLOSESQUARE { $$ = new std::vector<tf::Expr *>(); } 
         | OPENSQUARE exprtuple_ CLOSESQUARE { $$ = $2; }

lval: 
  IDENTIFIER {
    $$ = new tf::LValIdent(*$1);
  } | IDENTIFIER exprtuple {
    $$ = new tf::LValArray(*$1, *$2);
  }

type:
    basetype { $$ = $1; } 
    | basetype exprtuple {
     $$ = new tf::TypeArray(((tf::TypeBase*)$1)->t, *$2);
    }

basetype: 
    INTTY {
        $$ = new tf::TypeBase(tf::TypeBaseName::Int);
    } | FLOATTY {
        $$ = new tf::TypeBase(tf::TypeBaseName::Float);
    } | VOIDTY {
        $$ = new tf::TypeBase(tf::TypeBaseName::Void);
    } | FILETY {
        $$ = new tf::TypeBase(tf::TypeBaseName::File);
    } | CHARTY {
        $$ = new tf::TypeBase(tf::TypeBaseName::Char);
    }

stmt: 
    lval EQUALS expr SEMICOLON {
         $$ = new tf::StmtSet($1, $3);
    } | WHILE expr block {
         $$ = new tf::StmtWhileLoop($2, $3);
    } | IDENTIFIER COLON type SEMICOLON {
       $$ = new tf::StmtLet(*$1, $3);
    } | expr SEMICOLON {
         $$ = new tf::StmtExpr($1);
    } | IF expr block iftail {
        $$ = new tf::StmtIf($expr, $block, $iftail);
    } | RETURN expr SEMICOLON {
        $$ = new tf::StmtReturn($expr);
    }

iftail: 
      ELIF expr block iftail { 
        $$ = new tf::StmtIf($expr, $block, $4);
      } | ELSE block {
        $$ = new tf::StmtTailElse($block);
      }
      | %empty {
        $$ = nullptr;
      }

fnimport: IMPORT IDENTIFIER fnparams COLON type {
      std::vector<tf::Type *>paramtys;
      std::vector<std::string> formals;

      for(int i = 0; i < (int)$fnparams->size(); ++i) {
          paramtys.push_back((*$fnparams)[i].second);
          formals.push_back((*$fnparams)[i].first);
      }
      tf::TypeFn *tyfn = new tf::TypeFn($type, paramtys);
      $$ = new tf::FnImport(*$IDENTIFIER, formals, tyfn);
 }

varimport: IMPORT IDENTIFIER COLON type {
         $$ = new tf::VarImport(*$IDENTIFIER, $type);
 }


stmts: 
     stmts stmt {
         $$ = $1;
         $$->push_back($2);
     } | stmt {
       $$ = new std::vector<tf::Stmt*>();
       $$->push_back($1);
     }
     

fnparams: 
    OPENPAREN CLOSEPAREN {
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

fndefn: DEF IDENTIFIER fnparams COLON type block {
      std::vector<tf::Type *>paramtys;
      std::vector<std::string> formals;

      for(int i = 0; i < (int)$fnparams->size(); ++i) {
          paramtys.push_back((*$fnparams)[i].second);
          formals.push_back((*$fnparams)[i].first);
      }
      tf::TypeFn *tyfn = new tf::TypeFn($type, paramtys);
      $$ = new tf::FnDefn(*$IDENTIFIER, formals, tyfn, $block);
  }
%%

