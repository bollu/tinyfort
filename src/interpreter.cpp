#include <interpreter.h>
using namespace tf;
using namespace std;

struct State {
    Scope<std::string, InterpValue> scope;
};

void interpretFnDefn(FnDefn f, std::vector<InterpValue *> args, State &s);
InterpValue *interpretExpr(State &s, Expr *e);
void interpretStmt(State &s, Stmt *stmt);

InterpValue *interpretCall(State &s, std::string name,
                           std::vector<InterpValue *> args) {
    if (name == "readint64") {
        int i;
        cin >> i;
        return InterpValue::Int(i);
    } else if (name == "print") {
        for (InterpValue *v : args) {
            v->print(cout);
            cout << "\n";
            return InterpValue::Void();
        }
    } else {
        InterpValue *v = s.scope.find(name);
        if (v == nullptr) {
            cerr << "Unknown function call: |" << name << "|";
            assert(false && "unknown function call");
        }

        assert(v != nullptr);
        FnDefn *f = v->as_function();
        assert(f != nullptr && "illegal function in scope");
        interpretFnDefn(*f, args, s);

        assert(false && "unknown function call");
    }

    assert(false && "unreachable");
};

void InterpValue::print(std::ostream &o) {
    switch (this->type) {
        case InterpValueType::Int:
            o << this->i;
            break;
        case InterpValueType::Void:
            o << "void";
            break;
        case InterpValueType::Float:
        case InterpValueType::Array:
        case InterpValueType::Function:
        case InterpValueType::Reference:
            assert(false && "unimplemented print.");
    }
};

InterpValue *interpretBinop(State &s, ExprBinop *b) {
    InterpValue *l = interpretExpr(s, b->l);
    InterpValue *r = interpretExpr(s, b->r);

    assert(l != nullptr);
    assert(r != nullptr);
    switch (b->op) {
        case (BinopAdd): {
            if (l->is_int()) return InterpValue::Int(l->as_int() + r->as_int());
            assert(false && "unreachable");
            break;
        }
        case (BinopSub): {
            if (l->is_int()) return InterpValue::Int(l->as_int() - r->as_int());
            assert(false && "unreachable");
            break;
        }
        case (BinopMul): {
            if (l->is_int()) return InterpValue::Int(l->as_int() * r->as_int());
            assert(false && "unreachable");
            break;
        }

        case (BinopDiv):
        case (BinopModulo):
        case (BinopOr):
        case (BinopLt):
        case (BinopLeq):
        case (BinopGt):
        case (BinopGeq):
        case (BinopCmpEq):
        case (BinopCmpNeq):
        case (BinopAnd):
        case (BinopLeftShift):
        case (BinopBitwiseAnd):
            assert(false && "unimplemented binop");
    }

    assert(false && "unreachable");
}

InterpValue *interpLValUse(State &s, LVal *lv) {
    if (LValIdent *i = dynamic_cast<LValIdent *>(lv)) {
        if (InterpValue *v = s.scope.find(i->name)) {
            return v;
        } else {
            cerr << "unknown identifier |" << i->name << "|\n";
            assert(false && "unknown identifier");
        }
    } else {
        LValArray *a = dynamic_cast<LValArray *>(a);
        assert(false && "unimplemented use of array");
    }

    assert(false && "unreachable");
}

InterpValue *interpretExpr(State &s, Expr *e) {
    if (ExprInt *i = dynamic_cast<ExprInt *>(e)) {
        return InterpValue::Int(i->i);
    } else if (ExprFnCall *call = dynamic_cast<ExprFnCall *>(e)) {
        std::vector<InterpValue *> paramVals;
        for (Expr *p : call->params) {
            paramVals.push_back(interpretExpr(s, p));
        }

        return interpretCall(s, call->fnname, paramVals);
    } else if (ExprLVal *elv = dynamic_cast<ExprLVal *>(e)) {
        return interpLValUse(s, elv->lval);
    } else if (ExprBinop *b = dynamic_cast<ExprBinop *>(e)) {
        return interpretBinop(s, b);
    } else {
        cerr << "unknown expression: |";
        e->print(cerr);
        cerr << "|";
        assert(false && "unknown expression");
    }

    assert(false && "unreachable");
}

void interpretBlock(State &s, Block *b) {
    for (Stmt *stmt : b->stmts) {
        interpretStmt(s, stmt);
    }
}

void interpretStmt(State &s, Stmt *stmt) {
    if (StmtSet *stmtset = dynamic_cast<StmtSet *>(stmt)) {
        if (LValIdent *ident = dynamic_cast<LValIdent *>(stmtset->lval)) {
            s.scope.insert(ident->name, interpretExpr(s, stmtset->rhs));
        } else {
            LValArray *arr = dynamic_cast<LValArray *>(stmtset->lval);
            assert(false && "unimplementd");
        }

    } else if (dynamic_cast<StmtLet *>(stmt)) {
        // nothing to be done for a let.
        return;
    } else if (StmtLetSet *ls = dynamic_cast<StmtLetSet *>(stmt)) {
        s.scope.insert(ls->name, interpretExpr(s, ls->rhs));
    } else if (StmtExpr *e = dynamic_cast<StmtExpr *>(stmt)) {
        interpretExpr(s, e->e);
    } else if (StmtForLoop *f = dynamic_cast<StmtForLoop *>(stmt)) {
        interpretStmt(s, f->init);

        while (1) {
            const InterpValue *condv = interpretExpr(s, f->cond);
            const bool cond = condv->as_bool();
            if (!cond) break;

            interpretBlock(s, f->inner);
        }
        interpretStmt(s, f->after);
    } else {
        cerr << "unknown statement:\n";
        stmt->print(cerr);
        assert(false && "unknown statement");
    }
}

void interpretFnDefn(FnDefn f, std::vector<InterpValue *> args, State &s) {
    for (auto stmt : f.b->stmts) {
        interpretStmt(s, stmt);
    }
}

void interpret(Program *p) {
    State s;
    FnDefn *main = nullptr;
    for (FnDefn *f : p->fndefns) {
        if (f->name == "main") {
            main = f;
        }
        s.scope.insert(f->name, InterpValue::Function(f));
    }

    if (!main) {
        cerr << "WARNING: Unable to find main() function. not interpreting "
                "anything.\n";
    }

    interpretFnDefn(*main, {}, s);
};
