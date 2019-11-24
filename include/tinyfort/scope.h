#pragma once
#include <map>
#include <iostream>
#include "llvm/ADT/Optional.h"

using namespace llvm;
using namespace std;

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
        cerr << "Child:\n";
        if (inner) {
            inner.getValue()->dump(F, nesting + 1);
        }
        cerr << "identifiers(child level=" << nesting << "): \n";
        for (auto It : this->m) {
            F(It.first, It.second, nesting);
        }
    }

    ~Scope() {
        if (inner) delete inner.getValue();
    }
};
