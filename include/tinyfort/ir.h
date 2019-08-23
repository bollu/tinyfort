#pragma once
#include <vector>
#include <iostream>
#include <set>
#include <sstream>

using namespace std;

void align(std::ostream &o, int depth);
namespace tf{
    using Identifier = std::string;
    using ConstructorName = std::string;
    using TypeName = std::string;
    struct FnDefn { 
        std::string name;
        FnDefn(std::string name) : name(name) {};

        void print(std::ostream &o, int depth=0) {
            align(o, depth);
            o << "def " << name << "(" << ")";
        }

    };

    struct Program{
        std::vector<FnDefn*> fndefns;
        Program(std::vector<FnDefn*> fndefns) : fndefns(fndefns) {
        }
    };



}
