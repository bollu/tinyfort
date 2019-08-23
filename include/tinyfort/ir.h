#pragma once
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

void align(std::ostream &o, int depth);
namespace tf {
using Identifier = std::string;
using ConstructorName = std::string;
using TypeName = std::string;

enum Binop {
    BinopAdd,
    BinopSub,
    BinopMul,
    BinopDiv,
    BinopOr,
    BinopLt,
    BinopLeq,
    BinopGt,
    BinopGeq,
    BinopEq,
    BinopNeq
};
void printBinop(std::ostream &o, tf::Binop bp);

class Block;

class Expr {
   public:
    virtual void print(std::ostream &o, int depth = 0) = 0;
};

class ExprBinop : public Expr {
   public:
    Expr *l;
    Binop op;
    Expr *r;

    ExprBinop(Expr *l, Binop op, Expr *r) : l(l), op(op), r(r){};

    void print(std::ostream &o, int depth = 0) {
        o << "(";
        l->print(o, depth);
        o << " ";
        printBinop(o, op);
        o << " ";
        r->print(o, depth);
        o << ")";
    }
};

class ExprInt : public Expr {
   public:
    int i;
    ExprInt(int i) : i(i){};
    void print(std::ostream &o, int depth = 0) { o << i; }
};

class ExprIdent : public Expr {
   public:
    std::string s;
    ExprIdent(std::string s) : s(s){};

    void print(std::ostream &o, int depth = 0) { o << s; }
}

class Stmt {
   public:
    virtual void print(std::ostream &o, int depth = 0) = 0;
};

struct Block {
    std::vector<Stmt *> stmts;
    Block(std::vector<Stmt *> stmts) : stmts(stmts){};

    void print(std::ostream &o, int depth = 0) {
        align(o, depth);
        o << "{\n";
        for (auto s : stmts) {
            align(o, depth + 2);
            s->print(o, depth + 2);
            o << "\n";
        }
        align(o, depth);
        o << "}";
    }
};

class StmtSet : public Stmt {
   public:
    std::string lhs;
    Expr *rhs;
    StmtSet(std::string lhs, Expr *rhs) : lhs(lhs), rhs(rhs){};
    void print(std::ostream &o, int depth = 0) {
        o << "let ";
        o << lhs;
        o << " = ";
        rhs->print(o, depth);
        o << ";";
    }
};

class StmtWhileLoop : public Stmt {
   public:
    Expr *cond;
    Block *inner;
    StmtWhileLoop(Expr *cond, Block *inner) : cond(cond), inner(inner){};
    void print(std::ostream &o, int depth = 0) {
        o << "while ";
        cond->print(o, depth);
        inner->print(o, depth);
    }
};

struct FnDefn {
    std::string name;
    Block *b;
    FnDefn(std::string name, Block *b) : name(name), b(b){};

    void print(std::ostream &o, int depth = 0) {
        align(o, depth);
        o << "def " << name << "("
          << ") ";
        b->print(o, depth);
    }
};

struct Program {
    std::vector<FnDefn *> fndefns;
    Program(std::vector<FnDefn *> fndefns) : fndefns(fndefns) {}

    void print(std::ostream &o) {
        for (auto it : fndefns) {
            it->print(o);
            o << "\n";
        }
    }
};

}  // namespace tf
