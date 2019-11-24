#pragma once
#include <string>
#include "tinyfort/ir.h"
#include "tinyfort/scope.h"
enum class InterpValueType {
    Int, Float, Array, Reference, Function, Void
};

class InterpValue{
    InterpValueType type;
    int i;
    float f;
    std::vector<InterpValue> array;
    tf::FnDefn *function;
    InterpValue *reference;

    public:
    static InterpValue* Int(int i) {
        InterpValue *v = new InterpValue;
        v->i = i;
        v->type = InterpValueType::Int;
        return v;
    };

    static InterpValue* Void() {
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

    void print(std::ostream &o);
};

struct Interpreter{ 
    Scope<std::string, InterpValue*> values;
};

void interpret(tf::Program *p);
