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
        case tf::BinopModulo:
            o << "%";
            return;
        case tf::BinopOr:
            o << "||";
            return;
        case tf::BinopLt:
            o << "<";
            return;
        case tf::BinopGt:
            o << ">";
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
        case tf::BinopAnd:
            o << "&&";
            return;
        case tf::BinopBitwiseAnd:
          o << "&";
          return;
        case tf::BinopLeftShift:
          o << "<<";
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
            if (m.find(k) != m.end()) {
                cerr << "adding known key: |" << k << "| \n";
            }
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

Value *getOrInsertPrintString(llvm::Module &mod, Builder builder) {
    FunctionType *fty =
        FunctionType::get(builder.getVoidTy(), builder.getInt64Ty());
    Constant *c = mod.getOrInsertFunction("printstring", fty, {});
    return c;
}


Value *getOrInsertFputc(llvm::Module &mod, Builder builder) {
    FunctionType *fty = FunctionType::get(
        builder.getVoidTy(),
        {builder.getInt8Ty(), builder.getInt8Ty()->getPointerTo()}, false);
    Constant *c = mod.getOrInsertFunction("fputc", fty, {});
    return c;
}

Value *getOrInsertFputs(llvm::Module &mod, Builder builder) {
    FunctionType *fty = FunctionType::get(builder.getVoidTy(),
                                          {builder.getInt8Ty()->getPointerTo(),
                                           builder.getInt8Ty()->getPointerTo()},
                                          false);
    Constant *c = mod.getOrInsertFunction("fputs", fty, {});
    return c;
}

GlobalVariable *getOrInsertStdout(llvm::Module &mod, Builder builder) {
    GlobalVariable *v = mod.getGlobalVariable("stdout");
    if (v) return v;

    GlobalVariable *gstdout = new GlobalVariable(
        mod, builder.getInt8Ty()->getPointerTo(), /*isConstant=*/false,
        GlobalValue::ExternalLinkage, nullptr, "stdout");
    return gstdout;
}

Value *getMalloc(llvm::Module &mod, Builder builder) {
    FunctionType *fty =
        FunctionType::get(builder.getInt8PtrTy(), builder.getInt64Ty());
    Constant *c = mod.getOrInsertFunction("malloc", fty, {});
    return c;
}

Value *getMemcpy(llvm::Module &mod, Builder builder) {
    FunctionType *fty = FunctionType::get(
        builder.getVoidTy(),
        {builder.getInt8PtrTy(), builder.getInt8PtrTy(), builder.getInt64Ty()},
        false);
    Constant *c = mod.getOrInsertFunction("memcpy", fty, {});
    return c;
}

void createCallMemcpy(llvm::Module &mod, Builder builder, Value *Dest,
                      Value *Src, Value *NBytes) {
    Dest = builder.CreateBitOrPointerCast(Dest, builder.getInt8PtrTy());
    Src = builder.CreateBitOrPointerCast(Src, builder.getInt8PtrTy());
    NBytes = builder.CreateSExt(NBytes, builder.getInt64Ty());
    builder.CreateCall(getMemcpy(mod, builder), {Dest, Src, NBytes});
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
        // scope.insert("fputc",
        //              new SymValue(getOrInsertFputc(mod, builder),
        //                           /*TODO: add function types */ nullptr));

        // scope.insert("fputs",
        //              new SymValue(getOrInsertFputs(mod, builder),
        //                           /*TODO: add function types */ nullptr));

        // scope.insert("stdout", new SymValue(getOrInsertStdout(mod, builder),
        //                                     /*TODO: add file types*/
        //                                     nullptr));

        for (FnImport *import : p.fnimports) {
            scope.insert(
                import->name,
                new SymValue(this->codegenFnImport(scope, import, builder),
                             /*TODO: add function types */ nullptr));
        }
        for (VarImport *import : p.varimports) {
            scope.insert(
                import->name,
                new SymValue(this->codegenVarImport(scope, import, builder),
                             /*TODO: add function types */ nullptr));
        }

        for (FnDefn *fndefn : p.fndefns) {
            this->codegenFunction(scope, fndefn, builder);
        }
    }

    llvm::Type *getBaseLLVMType(const tf::TypeBaseName base) {
        switch (base) {
            case TypeBaseName::Int:
                return llvm::Type::getInt64Ty(ctx);
            case TypeBaseName::Float:
                return llvm::Type::getFloatTy(ctx);
            case TypeBaseName::Bool:
                return llvm::Type::getInt1Ty(ctx);
            case TypeBaseName::Void:
                return llvm::Type::getVoidTy(ctx);
            case TypeBaseName::Char:
                return llvm::Type::getInt8Ty(ctx);
            case TypeBaseName::File:
                return llvm::Type::getInt8PtrTy(ctx);
        }
        cerr << "unknown base type: |";
        tf::Type::printTypeBaseName(cerr, base);
        cerr << "|\n";
        assert(false && "unknown base type");
    }

    llvm::Type *getLLVMType(const tf::Type *t) {
        if (const tf::TypeArray *arr = dynamic_cast<const TypeArray *>(t)) {
            llvm::Type *base = getBaseLLVMType(arr->t);
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
            return getBaseLLVMType(bt->t);
        }
        assert(false && "uknown type");
    }

    // TODO: merge code with that of StmtSet
    llvm::Value *codegenLValUse(SymTable &scope, LVal *lval, Builder builder) {
        if (LValIdent *id = dynamic_cast<LValIdent *>(lval)) {
            SymValue *sv = scope.find(id->name);
            if (sv == nullptr) {
                cerr << "unable to find LVal: |";
                lval->print(cerr);
                cerr << "|\n";
            }
            assert(sv != nullptr);
            Value *V = sv->symValue;
            assert(V != nullptr);
            Value *LoadedV = builder.CreateLoad(V);
            // we have g : char[100]
            // puts(g);
            // so we cast to pointer
            if (LoadedV->getType()->isArrayTy()) {
                return builder.CreateBitOrPointerCast(
                    LoadedV,
                    LoadedV->getType()->getArrayElementType()->getPointerTo());
            } else {
                return LoadedV;
            }
        }
        // TODO: cleanup this godforsaken mess of code
        const LValArray *arr = dynamic_cast<LValArray *>(lval);
        assert(arr != nullptr);

        SymValue *sv = scope.find(arr->name);
        if (sv == nullptr) {
            errs() << "unable to find value: [" << arr->name << "]\n";
        }
        assert(sv != nullptr);
        Value *LV = sv->symValue;
        assert(LV != nullptr);
        llvm::SmallVector<llvm::Value *, 4> args;

        for (int i = 0; i < arr->indeces.size(); ++i) {
            args.push_back(codegenExpr(scope, arr->indeces[i], builder));
        }

        // generate array index
        assert(LV->getType()->isPointerTy());
        LValArray *larr = dynamic_cast<LValArray *>(lval);
        assert(sv != nullptr);
        // get the type of the array so we can access the dimension
        // sizes.
        TypeArray *tyarr = dynamic_cast<TypeArray *>(sv->symType);
        assert(tyarr != nullptr);

        assert(larr->indeces.size() == tyarr->sizes.size() &&
               "array indexed with different number of indeces than "
               "declaration");

        Value *CurStride = builder.getInt64(1);
        Value *CurIx = builder.getInt64(0);
        for (int i = 0; i < larr->indeces.size(); ++i) {
            Value *Size = builder.CreateSExt(codegenExpr(scope, tyarr->sizes[i], builder),
                                            builder.getInt64Ty());
            Value *Index = builder.CreateSExt(codegenExpr(scope, larr->indeces[i], builder),
                                            builder.getInt64Ty());
            CurIx = builder.CreateAdd(CurIx, builder.CreateMul(Index, CurStride));
            // TODO: this is kludgy. Let's not have symArrSizes. We can
            // regenerate this info when we want it.
            // CurStride = builder.CreateMul(sv->symArrSizes[i],
            // CurStride);
            CurStride = builder.CreateMul(Size, CurStride);
        }
        // int *arr = load int **arr_stackslot
        Value *Array = builder.CreateLoad(sv->symValue);
        // int *arr_at_ix = arr + index
        Value *Access = builder.CreateGEP(Array, CurIx);
        return builder.CreateLoad(Access);
    }

    llvm::Value *codegenExpr(SymTable &scope, Expr *e, Builder builder) {
        if (ExprInt *i = dynamic_cast<ExprInt *>(e)) {
            return builder.getInt64(i->i);
        } else if (ExprBool *b = dynamic_cast<ExprBool *>(e)) {
            return builder.getInt1(b->b);
        } else if (ExprChar *c = dynamic_cast<ExprChar *>(e)) {
            return builder.getInt8(c->c);
        } else if (ExprString *s = dynamic_cast<ExprString *>(e)) {
            return builder.CreateBitOrPointerCast(
                builder.CreateGlobalString(s->s.c_str()),
                builder.getInt8PtrTy());
        } else if (ExprLVal *lvale = dynamic_cast<ExprLVal *>(e)) {
            return codegenLValUse(scope, lvale->lval, builder);
        } else if (ExprFnCall *fncall = dynamic_cast<ExprFnCall *>(e)) {
            if (fncall->fnname == "print") {
                for (Expr *it : fncall->params) {
                    // TODO: we should use types to figure this out.
                    Value *Arg = codegenExpr(scope, it, builder);
                    if (Arg->getType()->isIntegerTy()) {
                        Value *fn = scope.find("printInt64")->symValue;
                        Arg = builder.CreateSExtOrTrunc(Arg,
                                                        builder.getInt64Ty());

                        builder.CreateCall(fn, {Arg});
                    }
                }  // end arguments loop
                return nullptr;
            }  // end fn == print
            SymValue *fnsv = scope.find(fncall->fnname);
            if (fnsv == nullptr) {
                errs() << "unable to find function: [" << fncall->fnname
                       << "]\n";
            }
            assert(fnsv != nullptr);

            llvm::SmallVector<llvm::Value *, 4> args;
            for (int i = 0; i < fncall->params.size(); ++i) {
                args.push_back(codegenExpr(scope, fncall->params[i], builder));
            }

            assert(fnsv->symValue != nullptr);
            return builder.CreateCall(fnsv->symValue, args);

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
                case Binop::BinopDiv:
                    return builder.CreateSDiv(l, r);
                case Binop::BinopModulo:
                    return builder.CreateSRem(l, r);
                case Binop::BinopLeq:
                    return builder.CreateICmpSLE(l, r);
                case Binop::BinopLt:
                    return builder.CreateICmpSLT(l, r);
                case Binop::BinopGt:
                    return builder.CreateICmpSGT(l, r);
                case Binop::BinopGeq:
                    return builder.CreateICmpSGE(l, r);
                case Binop::BinopAnd:
                    return builder.CreateAnd(l, r);
                case Binop::BinopOr:
                    return builder.CreateOr(l, r);
                case Binop::BinopCmpEq: 
                    return builder.CreateICmpEQ(l, r);
                case Binop::BinopCmpNeq: 
                    return builder.CreateICmpNE(l, r);
                 case Binop::BinopBitwiseAnd: 
                    return builder.CreateAnd(l, r);
                case Binop::BinopLeftShift: 
                    return builder.CreateShl(l, r);
                default:
                    cerr << "unknown binop: |" << eb->op << "|";
                    e->print(cerr);
                    assert(false && "uknown binop");
            }
        } else if (ExprCast *c = dynamic_cast<ExprCast *>(e)) {
            Value *tocast = codegenExpr(scope, c->e, builder);
            llvm::Type *castty = getLLVMType(c->castty);

            llvm::Instruction::CastOps castop =
                llvm::CastInst::getCastOpcode(tocast,
                                             /*isSigned=*/true,
                                             castty,
                                             /*isSigned=*/true);
            assert(llvm::CastInst::castIsValid(castop, tocast, castty) && "invalid cast");
            return builder.CreateCast(castop, tocast, castty);
        } else if (ExprNegate *en = dynamic_cast<ExprNegate *>(e)) {
            return builder.CreateNeg(codegenExpr(scope, en->e, builder));
        }

        cerr << "unable to codegen expression:\n[";
        e->print(cerr);
        cerr << "]";
        assert(false && "unknown expression to codegen");
    }

    // expects the builder to be at the correct location
    llvm::BasicBlock *codegenStmt(SymTable &scope, Stmt *stmt,
                                  llvm::BasicBlock *entry, Builder builder) {
        builder.SetInsertPoint(entry);

        if (StmtLetSet *letset = dynamic_cast<StmtLetSet *>(stmt)) {
            StmtLet let = StmtLet(letset->name, letset->ty);
            entry = codegenStmt(scope, &let, entry, builder);
            StmtSet set = StmtSet(new LValIdent(letset->name), letset->rhs);
            return codegenStmt(scope, &set, entry, builder);
        }
        if (StmtLet *let = dynamic_cast<StmtLet *>(stmt)) {
            llvm::Type *ty = getLLVMType(let->ty);
            llvm::Value *V = builder.CreateAlloca(ty);
            V->setName(let->name);
            // we need to codegen a malloc
            if (tf::TypeArray *arrty = dynamic_cast<tf::TypeArray *>(let->ty)) {
                assert(arrty != nullptr && "incorrect array type");


                std::vector<Value *> Sizes;
                for (auto it : arrty->sizes) {
                    Sizes.push_back(codegenExpr(scope, it, builder));
                }
                // add array into scope
                scope.insert(let->name, new SymValue(V, let->ty, Sizes));

                // compute array size
                // start by assuming 8 bytes, and then multiply with all the
                // sizes
                Value *Size = builder.getInt64(8);
                for (Value *Dimsize : Sizes) {
                    Size = builder.CreateMul(Size, Dimsize);
                }

                // malloc memory
                Value *MallocdMem =
                    builder.CreateCall(getMalloc(mod, builder), {Size});
                MallocdMem = builder.CreateBitCast(MallocdMem, ty);

                // store mallocd memory
                builder.CreateStore(MallocdMem, V);

            } else {
                // we have a regular value
                scope.insert(let->name, new SymValue(V, let->ty));
            }
            return entry;

        } else if (StmtSet *s = dynamic_cast<StmtSet *>(stmt)) {
            // we can have g = ...
            // where g is an array!
            if (LValIdent *lid = dynamic_cast<LValIdent *>(s->lval)) {
                SymValue *sv = scope.find(lid->name);
                assert(sv != nullptr);

                // g : char[10]
                // g = "abba"; g is registered as a LValIdent, but this is
                // really an array-copy instruction.
                if (tf::TypeArray *ta =
                        dynamic_cast<tf::TypeArray *>(sv->symType)) {
                    // array on the RHS, in this case, "abba";
                    // TODO: add checks that the type of LHS and type of RHS
                    // match
                    // Right now, we codegen a memcpy
                    Value *RightArr = codegenExpr(scope, s->rhs, builder);

                    // size = elemsize * [index sizes]
                    const int elemsize = mod.getDataLayout().getTypeStoreSize(
                        getBaseLLVMType(ta->t));
                    Value *Size = builder.getInt64(elemsize);
                    for (int i = 0; i < ta->sizes.size(); ++i) {
                        Size = builder.CreateMul(
                            Size, codegenExpr(scope, ta->sizes[i], builder));
                    }
                    // int **g_stackslot = sv->symvalue
                    // int *g_mem = *g_stackslot = LeftArr
                    Value *LeftArr = builder.CreateLoad(sv->symValue);

                    // create a memcpy of the RHS into the LHS of the size
                    // of the array.
                    createCallMemcpy(mod, builder, LeftArr, RightArr, Size);
                    return entry;

                } else {
                    assert(dynamic_cast<tf::TypeBase *>(sv->symType) &&
                           "need this to be basic type");
                    builder.CreateStore(codegenExpr(scope, s->rhs, builder),
                                        sv->symValue);
                    return entry;
                }
                assert(false && "unreachable");
            } else {
                LValArray *larr = static_cast<LValArray *>(s->lval);
                const std::string arrname = larr->name;
                const SymValue *sv = scope.find(arrname);
                assert(sv != nullptr);
                // get the type of the array so we can access the dimension
                // sizes.
                TypeArray *tyarr = dynamic_cast<TypeArray *>(sv->symType);
                assert(tyarr != nullptr);

                assert(larr->indeces.size() == tyarr->sizes.size() &&
                       "array indexed with different number of indeces than "
                       "declaration");

                Value *CurStride = builder.getInt64(1);
                Value *CurIx = builder.getInt64(0);
                for (int i = 0; i < larr->indeces.size(); ++i) {
                    Value *Index = builder.CreateSExt(codegenExpr(scope, larr->indeces[i], builder), builder.getInt64Ty());
                    Value *Size = builder.CreateSExt(codegenExpr(scope, tyarr->sizes[i], builder), builder.getInt64Ty());

                    CurIx = builder.CreateAdd(
                        CurIx,
                        builder.CreateMul(Index, CurStride));
                    // TODO: this is kludgy. Let's not have symArrSizes. We
                    // can regenerate this info when we want it. CurStride =
                    // builder.CreateMul(sv->symArrSizes[i], CurStride);
                    CurStride = builder.CreateMul(Size, CurStride);
                }
                // int *arr = load int **arr_stackslot
                Value *Array = builder.CreateLoad(sv->symValue);
                // int *arr_at_ix = arr + index
                Value *Access = builder.CreateGEP(Array, CurIx);
                // *arr_at_ix = val
                builder.CreateStore(codegenExpr(scope, s->rhs, builder),
                                    Access);
                return entry;
            }
        } else if (StmtWhileLoop *wh = dynamic_cast<StmtWhileLoop *>(stmt)) {
            scope.pushScope();

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

            // push a scope for the body of the while.
            scope.pushScope();
            BasicBlock *blockbb =
                codegenBlock(scope, wh->inner, bodybb, builder);
            scope.popScope();

            builder.SetInsertPoint(blockbb);
            builder.CreateBr(condbb);

            return exitbb;
        } else if (StmtForLoop *f = dynamic_cast<StmtForLoop *>(stmt)) {
            // push a scope for the declaration of for loop variable
            scope.pushScope();
            entry = codegenStmt(scope, f->init, entry, builder);

            Block innerWithAfter(f->inner->stmts);
            innerWithAfter.stmts.push_back(f->after);
            tf::StmtWhileLoop wh(f->cond, &innerWithAfter);
            // now codegen the while loop.
            BasicBlock *exit = codegenStmt(scope, &wh, entry, builder);
            scope.popScope();
            return exit;

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

            // push a scope for the then block.
            scope.pushScope();
            thenbb = codegenBlock(scope, sif->inner, thenbb, builder);
            scope.popScope();

            if (!thenbb->getTerminator()) {
                builder.SetInsertPoint(thenbb);
                builder.CreateBr(joinbb);
            }

            // TODO: check if we have a terminator, and eliminate everything
            // after a terminator.
            if (sif->tail) {
                // push a scope for the else branch.
                scope.pushScope();
                elsebb = codegenStmt(scope, sif->tail, elsebb, builder);
                scope.popScope();

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

    // codegen an extern here
    Constant *codegenFnImport(SymTable &scope, FnImport *f, Builder builder) {
        llvm::Type *retty = getLLVMType(f->ty->retty);
        SmallVector<llvm::Type *, 4> argtys;
        for (int i = 0; i < f->ty->paramsty.size(); ++i) {
            argtys.push_back(getLLVMType(f->ty->paramsty[i]));
        }

        FunctionType *fty = FunctionType::get(retty, argtys, false);
        return mod.getOrInsertFunction(f->name, fty, {});
    }

    GlobalVariable *codegenVarImport(SymTable &scope, VarImport *v,
                                     Builder builder) {
        llvm::Type *ty = getLLVMType(v->ty);

        return new GlobalVariable(mod, ty, /*isConstant=*/false,
                                  GlobalValue::ExternalLinkage, nullptr,
                                  v->name);
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
            Target->createTargetMachine(TargetTriple, CPU, Features, opt,
    RM); c.mod.setDataLayout(TM->createDataLayout());
    }
    */

    exit(0);

    return 0;
}
