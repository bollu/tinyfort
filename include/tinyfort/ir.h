#pragma once
#include <assert.h>
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
    BinopCmpEq,
    BinopCmpNeq
};
void printBinop(std::ostream &o, tf::Binop bp);

class Block;

enum TypeBaseName { Int, Float, Bool, File, Char, Void };

class Type {
   public:
    virtual void print(std::ostream &o, int depth = 0) = 0;

    static void printTypeBaseName(std::ostream &o, TypeBaseName t) {
        switch (t) {
            case Int:
                o << "int";
                return;
            case Float:
                o << "float";
                return;
            case Bool:
                o << "bool";
                return;
            case Void:
                o << "void";
                return;
            case File:
                o << "FILE"; 
            case Char:
                o << "char"; 
                return;
        }
        assert(false && "unreachable");
    }
};

class TypeBase : public Type {
   public:
    TypeBaseName t;

    TypeBase(TypeBaseName t) : t(t){};

    void print(std::ostream &o, int depth = 0) { printTypeBaseName(o, t); }
};

class TypeFn : public Type {
    public:
        Type *retty;
        std::vector<Type *>paramsty;

        TypeFn(Type *retty, std::vector<Type *>paramsty) : 
            retty(retty), paramsty(paramsty) { }


        void print(std::ostream &o, int depth = 0) { 
            o << "(";
            for(int i = 0; i < (int)paramsty.size(); ++ i) {
                paramsty[i]->print(o, depth);
                if (i < (int)paramsty.size() - 1) o << ", ";
            }
            o << ") -> ";
            retty->print(o, depth);
        }
};

class Expr {
   public:
    virtual void print(std::ostream &o, int depth = 0) = 0;
    virtual Type* getType() const = 0;
};

class TypeArray : public Type {
   public:
    TypeBaseName t;
    std::vector<Expr *> sizes;

    TypeArray(TypeBaseName t, std::vector<Expr *> sizes) : t(t), sizes(sizes) {}

    void print(std::ostream &o, int depth = 0) {
        printTypeBaseName(o, t);
        o << "[";
        for (unsigned i = 0; i < sizes.size(); ++i) {
            sizes[i]->print(o, depth);
            if (i < sizes.size() - 1) o << ", ";
        }
        o << "]";
    }
    Type* getType() const {
        assert(false && "unimplemented");
    }
};

class LVal {
   public:
    virtual Type *getType() const = 0;
    virtual void print(std::ostream &o, int depth = 0) = 0;
};

class LValIdent : public LVal {
   public:
    std::string s;
    LValIdent(std::string s) : s(s){};

    Type *getType() const {
        assert(false && "unimplemented");
    }

    void print(std::ostream &o, int depth = 0) { o << s; }
};

class LValArray : public LVal {
   public:
    std::string s;
    std::vector<Expr *> indeces;
    LValArray(std::string s, std::vector<Expr *> indeces)
        : s(s), indeces(indeces){};
    void print(std::ostream &o, int depth = 0) {
        o << s;
        o << "[";
        for (unsigned i = 0; i < indeces.size(); ++i) {
            indeces[i]->print(o, depth);
            if (i < indeces.size() - 1) o << ", ";
        }
        o << "]";
    }

    Type *getType() const {
        assert(false && "unimplemented");
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

    Type *getType() const {
        assert(false && "unimplemented");
    }
};

class ExprInt : public Expr {
   public:
    int i;
    ExprInt(int i) : i(i){};
    void print(std::ostream &o, int depth = 0) { o << i; }

    Type *getType() const {
        assert(false && "unimplemented");
    }
};

class ExprString : public Expr {
   public:
    std::string s;
    ExprString(std::string s) : s(s) {};
    void print(std::ostream &o, int depth = 0) { o << s; }

    Type *getType() const {
        assert(false && "unimplemented");
    }
};

class ExprChar : public Expr {
   public:
    char c;
    ExprChar(char c) : c(c) {};
    void print(std::ostream &o, int depth = 0) { o << "'" << c << "'"; }

    Type *getType() const {
        assert(false && "unimplemented");
    }
};

class ExprLVal : public Expr {
   public:
    LVal *lval;
    ExprLVal(LVal *lval) : lval(lval){};
    void print(std::ostream &o, int depth = 0) { lval->print(o, depth); }

    Type *getType() const {
        assert(false && "unimplemented");
    }
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

class StmtExpr : public Stmt {
   public:
    Expr *e;

    StmtExpr(Expr *e) : e(e){};
    void print(std::ostream &o, int depth = 0) {
        e->print(o, depth);
        o << ";";
    }
};


class StmtIf : public Stmt {
    public:
        Expr *cond;
        Block *inner;
        Stmt *tail;
        StmtIf(Expr *cond, Block *inner, Stmt *tail) : cond(cond),
        inner(inner), tail(tail) {};

    void print(std::ostream &o, int depth = 0) {
        o << "if ";
        cond->print(o, depth);
        inner->print(o,depth);
        if (tail) {
            o << "else";
            tail->print(o, depth);
        }
    }

};

class StmtTailElse : public Stmt {
    public:
        Block *inner;
        StmtTailElse(Block *inner) : inner(inner) {};
        void print(std::ostream &o, int depth = 0) {
            inner->print(o, depth);
        }
};

class StmtReturn : public Stmt {
    public:
        Expr *e;
        StmtReturn(Expr *e) : e(e) {};
        void print(std::ostream &o, int depth = 0) {
            o << "return ";
            e->print(o, depth);
        }
};



struct FnDefn {
    std::string name;
    std::vector<string> formals;
    TypeFn *ty;
    Block *b;
    FnDefn(std::string name, std::vector<string> formals,
           TypeFn *ty, Block *b)
        : name(name), formals(formals), ty(ty), b(b){

            assert(formals.size() == ty->paramsty.size());

        };

    void print(std::ostream &o, int depth = 0) {
        align(o, depth);
        o << "def " << name << "(";

        for (int i = 0; i < (int)formals.size(); ++i) {
            o << formals[i] << ":";
            ty->paramsty[i]->print(o);
            if (i < (int)formals.size() - 1) {
                o << ", ";
            }
        }

        o << ")";
        o << " : ";
        ty->retty->print(o);
        o << " ";
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
