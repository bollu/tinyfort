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
        case tf::BinopCmpEq:
            o << "==";
            return;
        case tf::BinopCmpNeq:
            o << "!=";
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
    Constant *c = mod.getOrInsertFunction("printint64", fty, {});
    return c;
}

Value *getMalloc(llvm::Module &mod, Builder builder) {
    FunctionType *fty =
        FunctionType::get(builder.getInt8PtrTy(), builder.getInt64Ty());
    Constant *c = mod.getOrInsertFunction("malloc", fty, {});
    return c;
}

struct SymValue {
    // value data
    llvm::Value *symValue;
    // value type.
    tf::Type *symType;
    std::vector<llvm::Value *> symArrSizes;

    SymValue(llvm::Value *symValue, tf::Type *symType)
        : symValue(symValue), symType(symType){};
    SymValue(llvm::Value *symValue, tf::Type *symType,
             std::vector<llvm::Value *> symArrSizes)
        : symValue(symValue), symType(symType), symArrSizes(symArrSizes){};
};

using SymTable = Scope<string, SymValue>;

struct Codegen {
    LLVMContext ctx;
    // map from variables to functions
    map<string, Function *> functions;
    // module
    llvm::Module mod;

    Codegen(tf::Program &p) : mod("main_module", ctx) {
        Builder builder(ctx);

        // scoped mapping from variables to values
        SymTable scope;
        scope.insert(
            "printInt64",
            new SymValue(getOrInsertPrintInt64(mod, builder),
                         /* TODO: add function types into IR */ nullptr));

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
            case TypeBaseName::Void:
                return llvm::Type::getVoidTy(ctx);
        }
    }

    llvm::Type *getLLVMType(const tf::Type *t) {
        if (const tf::TypeArray *arr = dynamic_cast<const TypeArray *>(t)) {
            llvm::Type *base = getBaseType(arr->t);
            return base->getPointerTo();
        } else if (const tf::TypeFn *fty = dynamic_cast<const TypeFn *>(t)) {
            llvm::SmallVector<llvm::Type *, 4> paramTypes;
            for (auto it : fty->paramsty) {
                paramTypes.push_back(getLLVMType(it));
            }

            return FunctionType::get(getLLVMType(fty->retty), paramTypes,
                                     /*isVarArg=*/false);

        } else {
            const tf::TypeBase *bt = dynamic_cast<const TypeBase *>(t);
            return getBaseType(bt->t);
        }
    }

    // TODO: merge code with that of StmtSet
    llvm::Value *codegenLValUse(SymTable &scope, LVal *lval, Builder builder) {
        cerr << __FUNCTION__ << " | codegening lval: ";
        lval->print(cerr);
        cerr << "\n";
        if (LValIdent *id = dynamic_cast<LValIdent *>(lval)) {
            SymValue *sv = scope.find(id->s);
            assert(sv != nullptr);
            Value *V = sv->symValue;
            assert(V != nullptr);
            errs() << __FUNCTION__ << " v: " << *V << "\n";
            return builder.CreateLoad(V);
        }

        const LValArray *arr = dynamic_cast<LValArray *>(lval);
        assert(arr != nullptr);

        if (arr->s == "print") {
            for (auto it : arr->indeces) {
                Value *Arg = codegenExpr(scope, it, builder);
                if (Arg->getType()->isIntegerTy()) {
                    Value *fn = scope.find("printInt64")->symValue;
                    Arg = builder.CreateSExtOrTrunc(Arg, builder.getInt64Ty());
                    Value *V = builder.CreateCall(fn, {Arg});
                    return V;
                }
            }
            return nullptr;
        }

        SymValue *sv = scope.find(arr->s);
        assert(sv != nullptr);
        Value *LV = sv->symValue;
        assert(LV != nullptr);
        llvm::SmallVector<llvm::Value *, 4> args;

        for (int i = 0; i < arr->indeces.size(); ++i) {
            args.push_back(codegenExpr(scope, arr->indeces[i], builder));
        }

        if (LV->getType()->isPointerTy() &&
            LV->getType()->getPointerElementType()->isFunctionTy()) {
            return builder.CreateCall(LV, args);
            // generate function call.
        } else {
            // generate array index
            assert(LV->getType()->isPointerTy());
            LValArray *larr = static_cast<LValArray *>(lval);
            const std::string arrname = larr->s;
            const SymValue *sv = scope.find(arrname);
            assert(sv != nullptr);
            // get the type of the array so we can access the dimension
            // sizes.
            TypeArray *tyarr = dynamic_cast<TypeArray *>(sv->symType);
            assert(tyarr != nullptr);

            assert(larr->indeces.size() == tyarr->sizes.size() && "array indexed with different number of indeces than declaration");

            Value *CurStride = builder.getInt32(1);
            Value *CurIx = builder.getInt32(0);
            for (int i = 0; i < larr->indeces.size(); ++i) {
                CurIx = builder.CreateAdd(CurIx, builder.CreateMul(codegenExpr(scope, larr->indeces[i], builder), CurStride));
                // TODO: this is kludgy. Let's not have symArrSizes. We can 
                // regenerate this info when we want it.
                // CurStride = builder.CreateMul(sv->symArrSizes[i], CurStride);
                CurStride = builder.CreateMul(codegenExpr(scope, tyarr->sizes[i], builder), CurStride);
            }
            // int *arr = load int **arr_stackslot
            Value *Array = builder.CreateLoad(sv->symValue);
            // int *arr_at_ix = arr + index
            Value *Access = builder.CreateGEP(Array, CurIx);
            return builder.CreateLoad(Access);
        }

        cerr << "unknown LVal: ";
        lval->print(cerr);
        assert(false && "unknown LVal");
        return nullptr;
    }

    llvm::Value *codegenExpr(SymTable &scope, Expr *e, Builder builder) {
        if (ExprInt *i = dynamic_cast<ExprInt *>(e)) {
            return builder.getInt32(i->i);
        } else if (ExprLVal *lvale = dynamic_cast<ExprLVal *>(e)) {
            return codegenLValUse(scope, lvale->lval, builder);
        } else if (ExprBinop *eb = dynamic_cast<ExprBinop *>(e)) {
            Value *l = codegenExpr(scope, eb->l, builder);
            Value *r = codegenExpr(scope, eb->r, builder);

            switch (eb->op) {
                case Binop::BinopAdd:
                    return builder.CreateAdd(l, r);
                case Binop::BinopSub:
                    return builder.CreateSub(l, r);
                case Binop::BinopMul:
                    return builder.CreateMul(l, r);
                case Binop::BinopLeq:
                    return builder.CreateICmpSLE(l, r);
                case Binop::BinopCmpEq: {
                    // this for some reason infinite loops.
                    return builder.CreateICmpSLE(l, r);
                }
                default:
                    cerr << "unknown binop: |" << eb->op << "|";
                    e->print(cerr);
                    assert(false && "uknown binop");
            }
        }

        cerr << "unable to codegen expression:\n";
        e->print(cerr);
        assert(false && "unknown expression to codegen");
    }

    // expects the builder to be at the correct location
    llvm::BasicBlock *codegenStmt(SymTable &scope, Stmt *stmt,
                                  llvm::BasicBlock *entry, Builder builder) {
        builder.SetInsertPoint(entry);

        if (StmtLet *let = dynamic_cast<StmtLet *>(stmt)) {
            llvm::Type *ty = getLLVMType(let->ty);
            llvm::Value *V = builder.CreateAlloca(ty);
            V->setName(let->name);
            // we need to codegen a malloc
            if (ty->isPointerTy()) {
                tf::TypeArray *arrty = static_cast<tf::TypeArray *>(let->ty);

                std::vector<Value *> Sizes;
                for (auto it : arrty->sizes) {
                    Sizes.push_back(codegenExpr(scope, it, builder));
                }
                // add array into scope
                scope.insert(let->name, new SymValue(V, let->ty, Sizes));

                // compute array size
                // start by assuming 8 bytes, and then multiply with all the sizes
                Value *Size = builder.getInt32(8);
                for (Value *Dimsize : Sizes) {
                    Size = builder.CreateMul(Size, Dimsize);
                }

                // malloc memory
                Value *MallocdMem = builder.CreateCall(getMalloc(mod, builder), 
                        {Size});
                MallocdMem = builder.CreateBitCast(MallocdMem, ty);

                // store mallocd memory
                builder.CreateStore(MallocdMem, V);

            } else {
                // we have a regular value
                scope.insert(let->name, new SymValue(V, let->ty));
            }
            return entry;

        } else if (StmtSet *s = dynamic_cast<StmtSet *>(stmt)) {
            if (LValIdent *lid = dynamic_cast<LValIdent *>(s->lval)) {
                SymValue *sv = scope.find(lid->s);
                assert(sv != nullptr);
                builder.CreateStore(codegenExpr(scope, s->rhs, builder),
                                    sv->symValue);
                return entry;
            } else {
                LValArray *larr = static_cast<LValArray *>(s->lval);
                const std::string arrname = larr->s;
                const SymValue *sv = scope.find(arrname);
                assert(sv != nullptr);
                // get the type of the array so we can access the dimension
                // sizes.
                TypeArray *tyarr = dynamic_cast<TypeArray *>(sv->symType);
                assert(tyarr != nullptr);

                assert(larr->indeces.size() == tyarr->sizes.size() && "array indexed with different number of indeces than declaration");

                Value *CurStride = builder.getInt32(1);
                Value *CurIx = builder.getInt32(0);
                for (int i = 0; i < larr->indeces.size(); ++i) {
                    CurIx = builder.CreateAdd(CurIx, builder.CreateMul(codegenExpr(scope, larr->indeces[i], builder), CurStride));
                    // TODO: this is kludgy. Let's not have symArrSizes. We can 
                    // regenerate this info when we want it.
                    // CurStride = builder.CreateMul(sv->symArrSizes[i], CurStride);
                    CurStride = builder.CreateMul(codegenExpr(scope, tyarr->sizes[i], builder), CurStride);
                }
                // int *arr = load int **arr_stackslot
                Value *Array = builder.CreateLoad(sv->symValue);
                // int *arr_at_ix = arr + index
                Value *Access = builder.CreateGEP(Array, CurIx);
                // *arr_at_ix = val
                builder.CreateStore(codegenExpr(scope, s->rhs, builder), Access);
                return entry;
            }

        } else if (StmtWhileLoop *wh = dynamic_cast<StmtWhileLoop *>(stmt)) {
            BasicBlock *condbb =
                llvm::BasicBlock::Create(ctx, "while.cond", entry->getParent());
            BasicBlock *bodybb =
                llvm::BasicBlock::Create(ctx, "while.body", entry->getParent());
            BasicBlock *exitbb =
                llvm::BasicBlock::Create(ctx, "while.exit", entry->getParent());

            builder.SetInsertPoint(entry);
            builder.CreateBr(condbb);

            builder.SetInsertPoint(condbb);
            Value *cond = codegenExpr(scope, wh->cond, builder);
            if (!bodybb->getTerminator())
                builder.CreateCondBr(cond, bodybb, exitbb);

            BasicBlock *blockbb =
                codegenBlock(scope, wh->inner, bodybb, builder);
            builder.SetInsertPoint(blockbb);
            builder.CreateBr(condbb);

            return exitbb;

        } else if (StmtIf *sif = dynamic_cast<StmtIf *>(stmt)) {
            BasicBlock *thenbb =
                llvm::BasicBlock::Create(ctx, "if.then", entry->getParent());
            BasicBlock *elsebb =
                llvm::BasicBlock::Create(ctx, "if.else", entry->getParent());
            BasicBlock *joinbb =
                llvm::BasicBlock::Create(ctx, "if.join", entry->getParent());

            builder.SetInsertPoint(entry);
            Value *cond = codegenExpr(scope, sif->cond, builder);
            builder.CreateCondBr(cond, thenbb, elsebb);

            thenbb = codegenBlock(scope, sif->inner, thenbb, builder);
            if (!thenbb->getTerminator()) {
                builder.SetInsertPoint(thenbb);
                builder.CreateBr(joinbb);
            }

            // TODO: check if we have a terminator, and eliminate everything
            // after a terminator.
            if (sif->tail) {
                elsebb = codegenStmt(scope, sif->tail, elsebb, builder);
                if (!elsebb->getTerminator()) {
                    builder.SetInsertPoint(elsebb);
                    builder.CreateBr(joinbb);
                }
            } else {
                builder.SetInsertPoint(elsebb);
                builder.CreateBr(joinbb);
            }

            return joinbb;

        } else if (StmtTailElse *te = dynamic_cast<StmtTailElse *>(stmt)) {
            return codegenBlock(scope, te->inner, entry, builder);
        } else if (StmtReturn *sret = dynamic_cast<StmtReturn *>(stmt)) {
            Value *V = codegenExpr(scope, sret->e, builder);
            builder.CreateRet(V);
            return entry;

        } else {
            StmtExpr *se = dynamic_cast<StmtExpr *>(stmt);
            assert(se != nullptr);
            codegenExpr(scope, se->e, builder);
            return entry;
        }

        assert(false && "should never reach here.");
    }

    // start to codegen from the given block
    llvm::BasicBlock *codegenBlock(SymTable &scope, Block *block,
                                   llvm::BasicBlock *entry, Builder builder) {
        for (auto stmt : block->stmts) {
            entry = codegenStmt(scope, stmt, entry, builder);
        }
        return entry;
    }

    Function *codegenFunction(SymTable &scope, FnDefn *f, Builder builder) {
        Function *F =
            Function::Create(cast<llvm::FunctionType>(getLLVMType(f->ty)),
                             GlobalValue::ExternalLinkage, f->name, &mod);
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx, "entry", F);
        builder.SetInsertPoint(entry);

        scope.pushScope();

        scope.insert(f->name, new SymValue(F, f->ty));

        auto arg = F->arg_begin();
        for (int i = 0; i < (int)f->formals.size(); ++i) {
            const std::string name = f->formals[i];
            tf::Type *ty = f->ty->paramsty[i];
            // create a slot of type T* to store the value of the mutable
            // parameter.
            Value *slot = builder.CreateAlloca(getLLVMType(ty));
            builder.CreateStore(arg, slot);

            scope.insert(name, new SymValue(slot, ty));
            arg++;
        }

        // TODO: add formal paramters into fnscope.
        // TODO: have a toplevel scope.
        BasicBlock *exitbb = codegenBlock(scope, f->b, entry, builder);

        // codegen "ret void" for a void function
        if (TypeBase *bt = dynamic_cast<TypeBase *>(f->ty->retty)) {
            if (bt->t == TypeBaseName::Void) {
                builder.SetInsertPoint(exitbb);
                builder.CreateRetVoid();
            }
        }

        return F;
    }
};

void writeModuleLLToFile(llvm::Module &m, const char *filepath) {
    std::error_code errcode;
    const std::string out_filepath = "out.ll";
    llvm::raw_fd_ostream outputFile(out_filepath, errcode,
                                    llvm::sys::fs::F_None);
    if (errcode) {
        std::cerr << "Unable to open output file: " << out_filepath << "\n";
        std::cerr << "Error: " << errcode.message() << "\n";
        exit(1);
    }
}

void debugVerifyModule(llvm::Module &m) {
    if (verifyModule(m, nullptr) == 1) {
        cerr << "vvvvvvv\n";
        errs() << m << "\n";
        cerr << "^^^Broken module found, aborting compilation. Error:vvv\n";
        verifyModule(m, &errs());
        exit(1);
    }
}

int compile_program(int argc, char **argv, tf::Program *p) {
    p->print(std::cerr);
    Codegen c(*p);

    debugVerifyModule(c.mod);
    if (argc >= 3) {
        writeModuleLLToFile(c.mod, argv[2]);
    } else {
        std::cerr
            << "No output filepath provided. writing module to stdout...\n";
        outs() << c.mod;
    }

    // Setup for codegen
    /*
    {
        // WTF? it says OOM
        InitializeNativeTarget();
        const std::string CPU = "generic";
        auto TargetTriple = sys::getDefaultTargetTriple();
        std::string Error;
        auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
        if (!Target) {
            errs() << Error;
            report_fatal_error("unable to lookup target");
        }

        const TargetOptions opt;
        auto RM = Optional<Reloc::Model>();
        const std::string Features = "";
        llvm::TargetMachine *TM =
            Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
        c.mod.setDataLayout(TM->createDataLayout());
    }
    */

    exit(0);

    return 0;
}
