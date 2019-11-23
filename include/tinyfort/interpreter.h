#pragma once
#include <string>
#include "tinyfort/ir.h"
#include "tinyfort/scope.h"
class InterpValue {};

struct Interpreter{ 
    Scope<std::string, InterpValue*> values;
};

void interpret(tf::Program *p);

