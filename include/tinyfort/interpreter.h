#pragma once
#include <string>
#include "tinyfort/ir.h"
#include "tinyfort/scope.h"
enum class InterpValueType {
    Int,
    Float,
    Array,
    Reference,
    Function,
    Void,
    Bool
};

class InterpValue {
    InterpValueType type;
    int i;
    float f;
    bool b;
    std::vector<InterpValue> array;
    tf::FnDefn *function;
    InterpValue *reference;

   public:
    static InterpValue *Int(int i) {
        InterpValue *v = new InterpValue;
        v->i = i;
        v->type = InterpValueType::Int;
        return v;
    };

    static InterpValue *Bool(bool b) {
        InterpValue *v = new InterpValue;
        v->type = InterpValueType::Bool;
        v->b = b;
        return v;
    }
    static InterpValue *Void() {
        InterpValue *v = new InterpValue;
        v->type = InterpValueType::Void;
        return v;
    }

    static InterpValue *Function(tf::FnDefn *fn) {
        InterpValue *v = new InterpValue;
        v->type = InterpValueType::Function;
        v->function = fn;
        return v;
    }

    int as_int() const {
        assert(type == InterpValueType::Int);
        return i;
    }

    bool as_bool() const {
        assert(type == InterpValueType::Bool);
        return b;
    }

    bool is_int() const { return (type == InterpValueType::Int); }
    bool is_float() const { return (type == InterpValueType::Float); }

    tf::FnDefn *as_function() const {
        assert(type == InterpValueType::Function);
        return function;
    }

    void print(std::ostream &o);
};

struct Interpreter {
    Scope<std::string, InterpValue *> values;
};

void interpret(tf::Program *p);
