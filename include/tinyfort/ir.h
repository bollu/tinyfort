#pragma once
#include <vector>
#include <iostream>
#include <set>
#include <sstream>

using namespace std;

void align(std::ostream &o, int depth);
namespace tf{
    using Identifier = std::string;
    using ConstructorName = std::string;
    using TypeName = std::string;

    class Expr {
        public:
            virtual void print(std::ostream &o, int depth=0) = 0;
    };

    class ExprInt : public Expr {
        public:
            int i;
            ExprInt(int i) : i (i) {};
            void print(std::ostream &o, int depth=0) {
                o << i;
            }
    };

    class Stmt {
        public:
            virtual void print(std::ostream &o, int depth=0) = 0;
    };
    class StmtSet : public Stmt{
        public:
            std::string lhs;
            Expr *rhs;
            StmtSet(std::string lhs, Expr *rhs) : lhs(lhs), rhs(rhs) {};
            void print(std::ostream &o, int depth=0) {
                o << "let ";
                o << lhs;
                o << " = ";
                rhs->print(o, depth);
                o << ";";
            }
    };

    struct Block {
        std::vector<Stmt*> stmts;
        Block(std::vector<Stmt*>stmts) : stmts(stmts) {};

        void print(std::ostream &o, int depth=0) {
            o << "{\n";
            for(auto s: stmts) {
                align(o, depth);
                s->print(o, depth+2);
            }
            o << "\n}";
        }
    };

    struct FnDefn { 
        std::string name;
        Block *b;
        FnDefn(std::string name, Block *b) : name(name), b(b) {};

        void print(std::ostream &o, int depth=0) {
            align(o, depth);
            o << "def " << name << "(" << ")" << "\n";
            b->print(o, 2);
        }

    };

    struct Program{
        std::vector<FnDefn*> fndefns;
        Program(std::vector<FnDefn*> fndefns) : fndefns(fndefns) {
        }
    };


}
