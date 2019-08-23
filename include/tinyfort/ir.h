#pragma once
#include <iostream>
#include <set>
#include <sstream>
#include <vector>
#include <assert.h>

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


enum TypeBaseName {
    Int,
    Float,
    Bool
};

class Type {
    public:
    virtual void print(std::ostream &o, int depth = 0) = 0;


    static void printTypeBaseName(std::ostream &o, TypeBaseName t) {
        switch(t) {
            case Int: o << "int"; return;
            case Float: o << "float"; return;
            case Bool: o << "bool"; return;
        }
        assert(false && "unreachable");
    }
};

class TypeBase : public Type {
public:
    TypeBaseName t;

    TypeBase(TypeBaseName t) : t(t) {};
    
    void print(std::ostream &o, int depth = 0) {
        printTypeBaseName(o, t);
    }
};

class Expr {
   public:
    virtual void print(std::ostream &o, int depth = 0) = 0;
};

class TypeArray : public Type {
public:
    TypeBaseName t;
    std::vector<Expr *> sizes;

    TypeArray(TypeBaseName t, std::vector<Expr *> sizes) : t(t), sizes(sizes)
    {}

    void print(std::ostream &o, int depth = 0) {
        printTypeBaseName(o, t);
        o << "[";
        for(unsigned i = 0; i < sizes.size(); ++i) {
            sizes[i]->print(o, depth);
            if (i < sizes.size() - 1) o << ", ";
        }
        o << "]";
    }
};

class LVal {
   public:
    virtual void print(std::ostream &o, int depth = 0) = 0;
};

class LValIdent : public LVal {
   public:
    std::string s;
    LValIdent(std::string s) : s(s){};

    void print(std::ostream &o, int depth = 0) { o << s; }
};


class LValArray : public LVal {
    public:
        std::string s;
        std::vector<Expr *> indeces;
        LValArray(std::string s, std::vector<Expr *> indeces)
            : s(s), indeces(indeces) {};
        void print(std::ostream &o, int depth = 0) { 
            o << s;
            o << "[";
            for(unsigned i = 0; i < indeces.size(); ++i) {
                indeces[i]->print(o, depth);
                if (i < indeces.size() - 1) o << ", ";
            }
            o << "]";
        }

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

class ExprLVal : public Expr {
   public:
    LVal *lval;
    ExprLVal(LVal *lval) : lval(lval){};
    void print(std::ostream &o, int depth = 0) { lval->print(o, depth); }
};

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

class StmtLet : public Stmt {
   public:
    std::string name;
    Type *ty;
    StmtLet(std::string name, Type *ty) : name(name), ty(ty){};
    void print(std::ostream &o, int depth = 0) {
        o << "let ";
        o << name;
        o << " : ";
        ty->print(o, depth);
        o << ";";
    }
};

class StmtSet : public Stmt {
   public:
    LVal *lval;
    Expr *rhs;
    StmtSet(LVal *lval, Expr *rhs) : lval(lval), rhs(rhs){};
    void print(std::ostream &o, int depth = 0) {
        lval->print(o, depth);
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
