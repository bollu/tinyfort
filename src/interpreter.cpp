#include <interpreter.h>
#include <stdio.h>
#include <stack>
using namespace tf;
using namespace std;

using Env = std::map<std::string, InterpValue *>;
struct State {
    vector<Env> scopes;
    // whether the state is one that returned a value
    Optional<InterpValue *> retval;
    State() : retval(None) {
        scopes.push_back(std::map<string, InterpValue *>());
    };
};

InterpValue *getValue(const State &s, std::string name) {
    for (int i = s.scopes.size() - 1; i >= 0; i--) {
        const Env &name2v = s.scopes[i];
        auto it = name2v.find(name);
        if (it != name2v.end()) return it->second;
    }
    return nullptr;
}

void createScalarSlot(State &s, std::string name) {
    assert(s.scopes.size() > 0);
    Env &env = s.scopes[s.scopes.size() - 1];
    env[name] = InterpValue::Void();
}

void createArraySlot(State &s, std::string name) {
    assert(s.scopes.size() > 0);
    Env &env = s.scopes[s.scopes.size() - 1];
    env[name] = InterpValue::Array({});
}

void createTypeBasedSlot(State &s, std::string name, Type *ty) {
    if (dynamic_cast<TypeArray *>(ty)) {
        createArraySlot(s, name);
    } else if (dynamic_cast<TypeBase *>(ty)) {
        createScalarSlot(s, name);
    } else {
        assert(false && "unknown type");
    }
}

void setValue(State &s, std::string name, InterpValue *value) {
    for (int i = s.scopes.size() - 1; i >= 0; i--) {
        Env &name2v = s.scopes[i];
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
    s.scopes.push_back(std::map<string, InterpValue *>());
}

void popScope(State &s) { s.scopes.pop_back(); }

InterpValue *interpretFnDefn(FnDefn f, std::vector<InterpValue *> args,
                             State &s);
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
            return InterpValue::Void();
        }
    } else if (name == "fputs") {
        assert(args.size() == 2);
        std::string s = args[0]->as_string();
        FILE *f = args[1]->as_file();
        fputs(s.c_str(), f);
        return InterpValue::Void();
    } else if (name == "fputc") {
        assert(args.size() == 2);
        char c = args[0]->as_char();
        FILE *f = args[1]->as_file();
        fputc(c, f);
        return InterpValue::Void();
    } else {
        InterpValue *v = getValue(s, name);
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

        case (BinopModulo): {
            if (l->is_int()) return InterpValue::Int(l->as_int() % r->as_int());

            assert(false && "unreachable");
            break;
        }

        case (BinopOr): {
            return InterpValue::Bool(l->as_bool() || r->as_bool());
        }
        case (BinopLt): {
            if (l->is_int())
                return InterpValue::Bool(l->as_int() <= r->as_int());

            assert(false && "unreachable");
        }
        case (BinopLeq): {
            if (l->is_int())
                return InterpValue::Bool(l->as_int() < r->as_int());

            assert(false && "unreachable");
            break;
        }

        case (BinopGt):
            assert(false);
        case (BinopGeq):
            assert(false);
        case (BinopCmpEq): {
            if (l->is_int())
                return InterpValue::Bool(l->as_int() == r->as_int());
            assert(false && "unreachable");
            break;
        }
        case (BinopCmpNeq): {
            if (l->is_int())
                return InterpValue::Bool(l->as_int() != r->as_int());
            assert(false && "unreachable");
            break;
        }
        case (BinopAnd): {
            return InterpValue::Bool(l->as_bool() && r->as_bool());
        }

        case (BinopLeftShift): {
            const int li = l->as_int();
            const int ri = r->as_int();
            return InterpValue::Int(li << ri);
        }
        case (BinopBitwiseAnd): {
            const int li = l->as_int();
            const int ri = r->as_int();
            return InterpValue::Int(li & ri);
        }
    }

    assert(false && "unreachable");
}

InterpValue *interpLValUse(State &s, LVal *lv) {
    if (LValIdent *i = dynamic_cast<LValIdent *>(lv)) {
        if (InterpValue *v = getValue(s, i->name)) {
            return v;
        } else {
            cerr << "unknown identifier |" << i->name << "|\n";
            assert(false && "unknown identifier");
        }
    } else {
        LValArray *a = dynamic_cast<LValArray *>(lv);
        assert(a != nullptr && "unknown lval type");
        InterpValue *v = getValue(s, a->name);
        assert(v != nullptr && "unable to find value for array");

        std::vector<int> ixs;
        for (int i = 0; i < a->indeces.size(); ++i) {
            ixs.push_back(interpretExpr(s, a->indeces[i])->as_int());
        }

        ArrVal &arr = v->as_array();
        auto it = arr.find(ixs);
        assert(it != arr.end());
        return it->second;
    }

    assert(false && "unreachable");
}

InterpValue *interpretExpr(State &s, Expr *e) {
    // cerr << "interpreting: |";
    // e->print(cerr);
    // cerr << "|\n";

    if (ExprInt *i = dynamic_cast<ExprInt *>(e)) {
        return InterpValue::Int(i->i);
    } else if (ExprBool *b = dynamic_cast<ExprBool *>(e)) {
        return InterpValue::Bool(b->b);
    } else if (ExprString *es = dynamic_cast<ExprString *>(e)) {
        return InterpValue::String(es->s);
    } else if (ExprChar *c = dynamic_cast<ExprChar *>(e)) {
        return InterpValue::Char(c->c);
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
            std::vector<int> ixs;
            for (Expr *e : arr->indeces) {
                ixs.push_back(interpretExpr(s, e)->as_int());
            }
            InterpValue *v = getValue(s, arr->name);
            assert(v != nullptr && "unable to find array");
            v->as_array()[ixs] = interpretExpr(s, stmtset->rhs);
            // assert(false && "unimplementd");
        }

    } else if (StmtLet *let = dynamic_cast<StmtLet *>(stmt)) {
        createTypeBasedSlot(s, let->name, let->ty);
        // nothing to be done for a let.
        return;
    } else if (StmtReturn *sr = dynamic_cast<StmtReturn *>(stmt)) {
        // initialize retval
        s.retval = interpretExpr(s, sr->e);
        return;
    } else if (StmtLetSet *ls = dynamic_cast<StmtLetSet *>(stmt)) {
        createTypeBasedSlot(s, ls->name, ls->ty);
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
    } else if (StmtIf *f = dynamic_cast<StmtIf *>(stmt)) {
        if (interpretExpr(s, f->cond)->as_bool()) {
            interpretStmtBlock(s, f->inner);
        } else if (f->tail) {
            interpretStmt(s, f->tail);
        }
    } else if (StmtTailElse *e = dynamic_cast<StmtTailElse *>(stmt)) {
        interpretStmtBlock(s, e->inner);
    } else {
        cerr << "unknown statement:\n";
        stmt->print(cerr);
        assert(false && "unknown statement");
    }
}

// fnDefn does not take State by reference since it's supposed to be a new
// scope.
InterpValue *interpretFnDefn(FnDefn f, std::vector<InterpValue *> args,
                             State &s) {
    pushScope(s);
    assert(args.size() == f.formals.size());
    for (int i = 0; i < args.size(); ++i) {
        createTypeBasedSlot(s, f.formals[i], f.ty->paramsty[i]);
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

State initState() {
    State s;
    createScalarSlot(s, "stdin");
    setValue(s, "stdin", InterpValue::File(stdin));

    createScalarSlot(s, "stdout");
    setValue(s, "stdout", InterpValue::File(stdout));

    createScalarSlot(s, "stderr");
    setValue(s, "stderr", InterpValue::File(stderr));

    return s;
}

void interpret(Program *p) {
    State s = initState();

    FnDefn *main = nullptr;
    for (FnDefn *f : p->fndefns) {
        if (f->name == "main") {
            main = f;
        }
        createScalarSlot(s, f->name);
        setValue(s, f->name, InterpValue::Function(f));
    }

    if (!main) {
        cerr << "WARNING: Unable to find main() function. not interpreting "
                "anything.\n";
    }

    interpretFnDefn(*main, {}, s);
};
