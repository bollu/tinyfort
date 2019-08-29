#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <string>
#include "tinyfort/ir.h"
// Z3
#include "z3.h"
// LLVM
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

using namespace tf;
using namespace llvm;

using Builder = IRBuilder<>;

void align(std::ostream &o, int depth) {
    for (int i = 0; i < depth; ++i) o << " ";
}

void tf::printBinop(std::ostream &o, tf::Binop bp) {
    switch (bp) {
        case tf::BinopAdd:
            o << "+";
            return;
        case tf::BinopSub:
            o << "-";
            return;
        case tf::BinopMul:
            o << "*";
            return;
        case tf::BinopDiv:
            o << "/";
            return;
        case tf::BinopOr:
            o << "||";
            return;
        case tf::BinopLeq:
            o << "<=";
            return;
        case tf::BinopGeq:
            o << ">=";
            return;
        default:
            assert(false && "unreachable");
    }
}

template <typename K, typename V>
class Scope {
   public:
    using MapTy = std::map<K, V *>;
    using iterator = typename MapTy::iterator;
    using const_iterator = typename MapTy::const_iterator;

   private:
    MapTy m;
    Optional<Scope<K, V> *> inner;

   public:
    Scope() : inner(None){};

    void insert(K k, V *v) {
        if (inner)
            inner.getValue()->insert(k, v);
        else {
            assert(m.find(k) == m.end());
            m.insert(std::make_pair(k, v));
        }
    }

    void replace(K k, V *v) {
        if (inner)
            inner.getValue()->replace(k, v);
        else {
            assert(m.find(k) != m.end());
            m[k] = v;
        }
    }

    iterator end() { return m.end(); }

    const_iterator end() const { return m.end(); }

    // If our inner scope has this value, give it. Otherwise, default
    // and give what we have.
    // TODO: find a way to reduce code duplication b/w the two find.
    V *find(K k) {
        if (inner) {
            V *v = inner.getValue()->find(k);
            if (v != nullptr) return v;
        }

        auto it = m.find(k);
        if (it == m.end()) {
            return nullptr;
        }
        return it->second;
    }

    void pushScope() {
        if (!inner) {
            inner = Optional<Scope<K, V> *>(new Scope());
        } else {
            inner.getValue()->pushScope();
        }
    }

    void popScope() {
        assert(inner && "calling popScope on the innermost scope");
        if (inner.getValue()->isInnermostScope()) {
            delete inner.getValue();
            inner = None;
        } else {
            inner.getValue()->popScope();
        }
    }

    bool isInnermostScope() const { return !inner; }

    template <typename FTy>
    void dump(FTy F, unsigned nesting = 0) const {
        errs() << "Child:\n";
        if (inner) {
            inner.getValue()->dump(F, nesting + 1);
        }
        errs() << "identifiers(child level=" << nesting << "): \n";
        for (auto It : this->m) {
            F(It.first, It.second, nesting);
        }
    }

    ~Scope() {
        if (inner) delete inner.getValue();
    }
};

Value *getOrInsertPrintInt64(llvm::Module &mod, Builder builder) {
    FunctionType *fty =
        FunctionType::get(builder.getVoidTy(), builder.getInt64Ty());
    errs() << "fty: " << *fty << "\n";
    Constant *c = mod.getOrInsertFunction("printint64", fty, {});
    errs() << "c: " << *c << "\n";
    return c;
}
// get the format string for a given type
std::string getFormatString(tf::TypeBase *t) {
    switch (t->t) {
        case tf::TypeBaseName::Int:
            return "%d";
        case tf::TypeBaseName::Float:
            return "%f";
        case TypeBaseName::Bool:
            return "%d";
    }
}

struct Codegen {
    LLVMContext ctx;
    // map from variables to functions
    map<string, Function *> functions;
    // module
    llvm::Module mod;

    Codegen(tf::Program &p) : mod("main_module", ctx) {
        Builder builder(ctx);

        // scoped mapping from variables to values
        Scope<string, Value> scope;
        scope.insert("printInt64", getOrInsertPrintInt64(mod, builder));

        for (auto it : p.fndefns) {
            this->codegenFunction(scope, it, builder);
        }
    }

    llvm::Type *getBaseType(const tf::TypeBaseName base) {
        switch (base) {
            case TypeBaseName::Int:
                return llvm::Type::getInt32Ty(ctx);
            case TypeBaseName::Float:
                return llvm::Type::getFloatTy(ctx);
            case TypeBaseName::Bool:
                return llvm::Type::getInt1Ty(ctx);
        }
    }

    llvm::Type *getType(const tf::Type *t) {
        if (const tf::TypeArray *arr = dynamic_cast<const TypeArray *>(t)) {
            llvm::Type *base = getBaseType(arr->t);
            return base->getPointerTo();
        } else {
            const tf::TypeBase *bt = dynamic_cast<const TypeBase *>(t);
            return getBaseType(bt->t);
        }
    }

    FunctionType *getFunctionType(const FnDefn *f, Builder builder) {
        llvm::SmallVector<llvm::Type *, 4> paramTypes;
        for (auto it : f->formals) {
            paramTypes.push_back(getType(it.second));
        }

        return FunctionType::get(builder.getVoidTy(), paramTypes,
                                 /*isVarArg=*/false);
    };

    llvm::Value *codegenLVal(Scope<string, Value> &scope, LVal *lval,
                             Builder builder) {
        if (LValIdent *id = dynamic_cast<LValIdent *>(lval)) {
            Value *V = scope.find(id->s);
            return builder.CreateLoad(V);
        }

        const LValArray *arr = dynamic_cast<LValArray *>(lval);
        assert(arr);

        if (arr->s == "print") {
            errs() << "PRINTING..\n";
            for (auto it : arr->indeces) {
                Value *Arg = codegenExpr(scope, it, builder);
                if (Arg->getType()->isIntegerTy()) {
                    Value *fn = scope.find("printInt64");
                    Arg = builder.CreateSExtOrTrunc(Arg, builder.getInt64Ty());
                    errs() << "fn: " << *fn << " | arg: " << *Arg << "\n";
                    Value *V = builder.CreateCall(fn, {Arg});
                    errs() << "fncall : "<< *V << "\n";
                }
            }
            return nullptr;
        }

        Value *LV = scope.find(arr->s);
        assert(LV != nullptr);
        llvm::SmallVector<llvm::Value *, 4> args;

        if (LV->getType()->isFunctionTy()) {
            // generate function call.
        } else {
            // generate array index
            assert(LV->getType()->isArrayTy());
        }
        return nullptr;
    }

    llvm::Value *codegenExpr(Scope<string, Value> &scope, Expr *e,
                             Builder builder) {
        if (ExprInt *i = dynamic_cast<ExprInt *>(e)) {
            return builder.getInt32(i->i);
        } else if (ExprLVal *lvale = dynamic_cast<ExprLVal *>(e)) {
            return codegenLVal(scope, lvale->lval, builder);
        } else if (ExprBinop *eb = dynamic_cast<ExprBinop *>(e)) {
            Value *l = codegenExpr(scope, eb->l, builder);
            Value *r = codegenExpr(scope, eb->r, builder);

            switch (eb->op) {
                case Binop::BinopAdd:
                    return builder.CreateAdd(l, r);
                case Binop::BinopSub:
                    return builder.CreateSub(l, r);
            }
        }

        cerr << "unable to codegen expression:\n";
        e->print(cerr);
        assert(false && "unknown expression to codegen");
    }

    // expects the builder to be at the correct location
    llvm::BasicBlock *codegenStmt(Scope<string, Value> &scope, Stmt *stmt,
                                  llvm::BasicBlock *entry, Builder builder) {
        builder.SetInsertPoint(entry);

        if (StmtLet *let = dynamic_cast<StmtLet *>(stmt)) {
            llvm::Type *ty = getType(let->ty);
            if (ty->isArrayTy()) {
                assert(0 && "unknown how to codegen arrays");
            } else {
                llvm::Value *V = builder.CreateAlloca(ty);
                V->setName(let->name);
                scope.insert(let->name, V);
            }
            return entry;

        } else if (StmtSet *s = dynamic_cast<StmtSet *>(stmt)) {
            if (LValIdent *lid = dynamic_cast<LValIdent *>(s->lval)) {
                Value *v = scope.find(lid->s);
                assert(v != nullptr);
                builder.CreateStore(codegenExpr(scope, s->rhs, builder), v);
            } else {
                LValArray *larr = static_cast<LValArray *>(s->lval);
                (void)(larr);
                assert(0 && "unknown how to codegen arrays");
            }
            return entry;

        } else if (StmtWhileLoop *wh = dynamic_cast<StmtWhileLoop *>(stmt)) {
        } else {
            StmtExpr *se = dynamic_cast<StmtExpr *>(stmt);
            assert(se != nullptr);
            codegenExpr(scope, se->e, builder);
        }

        return entry;
    }

    // start to codegen from the given block
    llvm::BasicBlock *codegenBlock(Scope<string, Value> &scope, Block *block,
                                   llvm::BasicBlock *entry, Builder builder) {
        for (auto stmt : block->stmts) {
            entry = codegenStmt(scope, stmt, entry, builder);
        }
        return entry;
    }

    Function *codegenFunction(Scope<string, Value> &scope, FnDefn *f,
                              Builder builder) {
        Function *F =
            Function::Create(getFunctionType(f, builder),
                             GlobalValue::ExternalLinkage, f->name, &mod);
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", F);
        builder.SetInsertPoint(entry);
        // TODO: add formal paramters into fnscope.
        // TODO: have a toplevel scope.
        codegenBlock(scope, f->b, entry, builder);
        return F;
    }
};

int compile_program(tf::Program *p) {
    p->print(std::cout);
    Codegen c(*p);
    c.mod.print(errs(), nullptr);
    return 0;
}
