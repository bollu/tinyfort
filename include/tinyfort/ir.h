#include <iostream>
#include <set>
#include <sstream>

using namespace std;

namespace tf{
    using Identifier = std::string;
    using ConstructorName = std::string;
    using TypeName = std::string;
    struct Program{};
    struct FnDefn { 
        std::string name;
        FnDefn(std::string name) : name(name) {};
    };

}
