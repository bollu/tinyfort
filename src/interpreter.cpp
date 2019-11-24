#include <interpreter.h>
#include<stack>
using namespace tf;
using namespace std;

using Env = std::map<std::string, InterpValue*>;
struct State {
    vector<Env> scopes;
    // whether the state is one that returned a value
    Optional<InterpValue *> retval;
    State () : retval(None) {
        scopes.push_back(std::map<string, InterpValue*>());
    };
};

InterpValue *findValue(const State &s, std::string name) {
    for(int i = s.scopes.size() - 1; i >= 0; i--) {
        const Env &name2v  = s.scopes[i];
        auto it = name2v.find(name);
        if (it != name2v.end())
            return it->second;
    }
    return nullptr;
}

void createValueSlot(State &s, std::string name) {
    cerr << __FUNCTION__ << ": 1 "; cerr.flush();
    assert(s.scopes.size() > 0);
    cerr << "2 "; cerr.flush();
    Env &env = s.scopes[s.scopes.size() - 1];
    cerr << "3 "; cerr.flush();
    assert(env.find(name) == env.end());
    cerr << "4 "; cerr.flush();
    env[name] = InterpValue::Void();
    cerr << "5 "; cerr.flush();
    cerr << "\n";
}

void setValue(State &s, std::string name, InterpValue *value) {
    for(int i = s.scopes.size() - 1; i >= 0; i--) {
        Env &name2v  = s.scopes[i];
        auto it = name2v.find(name);
        if (it != name2v.end()) {
            name2v[name] = value;
            return;
        }
    }
    cerr << "unable to find variable: |" << name << "|\n";
    assert(false && "unable to find variable");
}

void pushScope(State &s) {
    s.scopes.push_back(std::map<string, InterpValue*>());
}

void popScope(State &s) {
    s.scopes.pop_back();
}

InterpValue *interpretFnDefn(FnDefn f, std::vector<InterpValue *> args, State &s);
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
        InterpValue *v = findValue(s, name);
        if (v == nullptr) {
            cerr << "Unknown function call: |" << name << "|";
            assert(false && "unknown function call");
        }

        assert(v != nullptr);
        FnDefn *f = v->as_function();
        assert(f != nullptr && "illegal function in scope");
        return interpretFnDefn(*f, args, s);

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

        case (BinopDiv): {
            if (l->is_int()) return InterpValue::Int(l->as_int() / r->as_int());
            assert(false && "unreachable");
            break;
        }

        case (BinopModulo):
        case (BinopOr):
        case (BinopLt):
        case (BinopLeq):  {
            if (l->is_int()) return InterpValue::Bool(l->as_int() <= r->as_int());
            
            assert(false && "unreachable");
            break;
        }

        case (BinopGt):
        case (BinopGeq):
        case (BinopCmpEq):
        case (BinopCmpNeq):
        case (BinopAnd):
        case (BinopLeftShift):
        case (BinopBitwiseAnd): {
            cerr << "Binop: |";
            b->print(cerr);
            cerr << "|";
            assert(false && "unimplemented binop");
        }
    }

    assert(false && "unreachable");
}

InterpValue *interpLValUse(State &s, LVal *lv) {
    if (LValIdent *i = dynamic_cast<LValIdent *>(lv)) {
        if (InterpValue *v = findValue(s, i->name)) {
            return v;
        } else {
            cerr << "unknown identifier |" << i->name << "|\n";
            assert(false && "unknown identifier");
        }
    } else {
        LValArray *a = dynamic_cast<LValArray *>(lv);
        assert(false && "unimplemented use of array");
    }

    assert(false && "unreachable");
}

InterpValue *interpretExpr(State &s, Expr *e) {
    // cerr << "interpreting: |";
    // e->print(cerr);
    // cerr << "|\n";

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

void interpretStmtBlock(State &s, Block *b) {
    for (Stmt *stmt : b->stmts) {
        interpretStmt(s, stmt);
        if (s.retval) return;
    }
}

void interpretStmt(State &s, Stmt *stmt) {
    // cerr << "interpreting: |";
    // stmt->print(cerr);
    // cerr << "|\n";

    if (StmtSet *stmtset = dynamic_cast<StmtSet *>(stmt)) {
        if (LValIdent *ident = dynamic_cast<LValIdent *>(stmtset->lval)) {
            setValue(s, ident->name, interpretExpr(s, stmtset->rhs));
        } else {
            LValArray *arr = dynamic_cast<LValArray *>(stmtset->lval);
            assert(false && "unimplementd");
        }

    } else if (StmtLet *let = dynamic_cast<StmtLet *>(stmt)) {
        createValueSlot(s, let->name);
        // nothing to be done for a let.
        return;
    } else if (StmtReturn *sr = dynamic_cast<StmtReturn *>(stmt)) {
        cerr << "stmt: " << stmt << " | retval: " << interpretExpr(s, sr->e)->as_int() << "\n";
        // initialize retval
        s.retval = interpretExpr(s, sr->e);
        return;
    }
    else if (StmtLetSet *ls = dynamic_cast<StmtLetSet *>(stmt)) {
        createValueSlot(s, ls->name);
        setValue(s, ls->name, interpretExpr(s, ls->rhs));
    } else if (StmtExpr *e = dynamic_cast<StmtExpr *>(stmt)) {
        interpretExpr(s, e->e);
    } else if (StmtForLoop *f = dynamic_cast<StmtForLoop *>(stmt)) {
        pushScope(s);
        interpretStmt(s, f->init);

        while (1) {
            const InterpValue *condv = interpretExpr(s, f->cond);
            const bool cond = condv->as_bool();
            if (!cond) break;
            interpretStmtBlock(s, f->inner);
            if (s.retval) return;
            interpretStmt(s, f->after);
        }
        popScope(s);
    } else {
        cerr << "unknown statement:\n";
        stmt->print(cerr);
        assert(false && "unknown statement");
    }
}

// fnDefn does not take State by reference since it's supposed to be a new
// scope.
InterpValue* interpretFnDefn(FnDefn f, std::vector<InterpValue *> args, State &s) {
    pushScope(s);
    assert(args.size() == f.formals.size());
    for (int i = 0; i < args.size(); ++i) {
        createValueSlot(s, f.formals[i]);
        setValue(s, f.formals[i], args[i]);
    }

    for (auto stmt : f.b->stmts) {
        interpretStmt(s, stmt);
        if (s.retval) {
            goto TERMINATE;
        }
    }

    TERMINATE:
    popScope(s);
    if (!s.retval) {
        return InterpValue::Void();
    } else {
        return *s.retval;
    }
}

void interpret(Program *p) {
    State s;
    FnDefn *main = nullptr;
    for (FnDefn *f : p->fndefns) {
        if (f->name == "main") {
            main = f;
        }
        createValueSlot(s, f->name);
        setValue(s, f->name, InterpValue::Function(f));
    }

    if (!main) {
        cerr << "WARNING: Unable to find main() function. not interpreting "
                "anything.\n";
    }

    interpretFnDefn(*main, {}, s);
};
