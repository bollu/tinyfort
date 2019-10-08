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
%token MODULO
%token LT
%token GT
%token LEQ;
%token CMPEQ;
%token CMPNEQ;
%token GEQ;
%token AND;
%token OR;
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
%token TRUE;
%token FALSE;

%start toplevel
%type <block>	block;
%type <stmt>	stmt;
%type <stmt>	iftail;
%type <stmt>	stmtletset;
%type <stmt>	stmtlet;
%type <stmt>	stmtset;
%type <lval>	lval;
%type <expr>	expr;
%type <expr>	expr2;
%type <expr>	expr3;
%type <expr>	fncall;
%type <expr>	expr4;
%type <expr>	expr5;
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
%token <str>  CHAR;
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

// logical
expr: expr2 AND expr2 { $$ = new tf::ExprBinop($1, tf::Binop::BinopAnd, $3); }
     | expr2 OR expr2 { $$ = new tf::ExprBinop($1, tf::Binop::BinopOr, $3); }
     | expr2 { $$ = $1; }

// relational
expr2: 
     expr3 LEQ expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopLeq, $3); }
     | expr3 CMPEQ expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopCmpEq, $3); }
     | expr3  { $$ = $1; }

// low precedence arithmetic
expr3 : expr4 { $$ = $1;
     }
     | expr4 PLUS expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopAdd, $3); }
     | expr4 MINUS expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopSub, $3); }
     | expr4 MODULO expr3 { $$ = new tf::ExprBinop($1, tf::Binop::BinopModulo, $3); }

// * , /
expr4: 
  expr5 STAR expr4 { $$ = new tf::ExprBinop($1, tf::Binop::BinopMul, $3); }
  | expr5 DIVIDE expr4 { $$ = new tf::ExprBinop($1, tf::Binop::BinopDiv, $3); }
  | expr5 { $$ = $1; }



// root literals
expr5: 
     OPENPAREN expr CLOSEPAREN { $$ = $2; }
     |  INTEGER { $$ = new tf::ExprInt($1); } 
     | STRING { $$ = new tf::ExprString(*$STRING); }
     | lval { $$ = new tf::ExprLVal($1); }
     | CHAR { $$ = new tf::ExprChar(*$CHAR); }
     | TRUE { $$ = new tf::ExprBool(true); }
     | FALSE { $$ = new tf::ExprBool(false); }
     | fncall { $$ = $1; }

// function calls
fncall : IDENTIFIER OPENPAREN CLOSEPAREN {
       $$ = new tf::ExprFnCall(*$IDENTIFIER);
       } | IDENTIFIER OPENPAREN exprtuple_ CLOSEPAREN {
       $$ = new tf::ExprFnCall(*$IDENTIFIER, *$exprtuple_);
     }

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
    } | BOOLTY {
        $$ = new tf::TypeBase(tf::TypeBaseName::Bool);
    }

stmtlet: IDENTIFIER COLON type {
       $$ = new tf::StmtLet(*$1, $3);
    }

stmtset: lval EQUALS expr {
         $$ = new tf::StmtSet($1, $3);
    }

stmtletset: IDENTIFIER COLON type EQUALS expr {
    $$ = new tf::StmtLetSet(*$IDENTIFIER, $type, $expr);
  }

stmt: stmtlet SEMICOLON {
        $$ = $stmtlet;
    } | stmtset SEMICOLON {
        $$ = $stmtset;
    } | stmtletset SEMICOLON {
      $$ = $stmtletset;
    } | WHILE expr block {
         $$ = new tf::StmtWhileLoop($2, $3);
    } |  expr SEMICOLON {
         $$ = new tf::StmtExpr($1);
    } | IF expr block iftail {
        $$ = new tf::StmtIf($expr, $block, $iftail);
    } | RETURN expr SEMICOLON {
        $$ = new tf::StmtReturn($expr);
    } | FOR stmtletset SEMICOLON expr SEMICOLON stmtset block {
        $$ = new tf::StmtForLoop($stmtletset, $expr, $stmtset, $block);
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

