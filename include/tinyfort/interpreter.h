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
    Bool,
    File,
    Char
};

class InterpValue;

using Index =  std::vector<int>;
using ArrVal = std::map<Index, InterpValue *>;

class InterpValue {
    InterpValueType type;
    int i;
    float f;
    bool b;
    char c;
    FILE *file;
    ArrVal array;
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

    static InterpValue *Array(std::map<Index, InterpValue *>arr) {
        InterpValue *v = new InterpValue;
        v->type = InterpValueType::Array;
        v->array = arr;
        return v;
    }


    static InterpValue *File(FILE *file) {
        InterpValue *v = new InterpValue;
        v->type = InterpValueType::File;
        v->file = file;
        return v;
    }

    static InterpValue *Char(char c) {
        InterpValue *v = new InterpValue;
        v->type = InterpValueType::Char;
        v->c = c;
        return v;
    }


    static InterpValue *String(std::string s) {
        std::map<Index, InterpValue *> encodedStr;
        for(int i = 0; i < s.size(); ++i) {
            Index ix = {i};
            encodedStr[ix] = InterpValue::Char(s[i]);
        }
        return InterpValue::Array(encodedStr);
    }


    ArrVal& as_array() {
        assert(type == InterpValueType::Array);
        return array;
    }

    int as_int() const {
        assert(type == InterpValueType::Int);
        return i;
    }

    bool as_bool() const {
        assert(type == InterpValueType::Bool);
        return b;
    }

    string as_string() const { 
        assert(type == InterpValueType::Array);
        std::string s;
        for(int i = 0; i < this->array.size(); ++i) {
            Index ix = {i};
            auto it = this->array.find(ix);
            assert(it != this->array.end());
            s += it->second->as_char();
        }
        return s;
    }

    FILE *as_file() const {
        assert(type == InterpValueType::File);
        return file;
    }

    char as_char() const {
        assert(type == InterpValueType::Char);
        return c;
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

void interpret(tf::Program *p, int argc, char **argv);
