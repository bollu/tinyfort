#include <interpreter.h>
using namespace tf;
using namespace std;

struct State {
    Scope<std::string, InterpValue> scope;
};

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
        assert(false && "unknown function call");
    }
    assert(false && "unreachable");
};

void InterpValue::print(std::ostream &o) {
    switch(this->type) {
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

InterpValue *interpretExpr(State &s, Expr *e) {
    if (ExprInt *i = dynamic_cast<ExprInt*>(e)) {
        return InterpValue::Int(i->i);
    }
    else if (ExprFnCall *call = dynamic_cast<ExprFnCall*>(e)) {
        std::vector<InterpValue *>paramVals;
        for(Expr *p : call->params) {
            paramVals.push_back(interpretExpr(s, p));
        }

        interpretCall(s, call->fnname, paramVals);
    }
    else {
        cerr << "unknown expression:\n";
        e->print(cerr);
        assert(false && "unknown expression");
    }

    assert(false && "unreachable");
}

void interpretStmt(State &s, Stmt *stmt) {
    if (StmtSet *stmtset = dynamic_cast<StmtSet *>(stmt)) {
        if (LValIdent *ident = dynamic_cast<LValIdent *>(stmtset->lval)) {
            s.scope.insert(ident->name, interpretExpr(s, stmtset->rhs));
        }
        else {
            LValArray *arr = dynamic_cast<LValArray *>(stmtset->lval);
            assert(arr);
        }

    }
    else if (StmtExpr *e = dynamic_cast<StmtExpr *>(stmt)) {
        interpretExpr(s, e->e);
    }
    else {
        cerr << "unknown statement:\n";
        stmt->print(cerr);
        assert(false && "unknown statement");
    }
}

State interpretFunction(FnDefn f, 
            std::vector<InterpValue *> args, State s) {
    for(auto stmt : f.b->stmts) {
        interpretStmt(s, stmt);
    }
    return s;
}

void interpret(Program *p) {
    State s;
    for(FnDefn *f : p->fndefns) {
        s.scope.insert(f->name, InterpValue::Function(f));
    }
};
