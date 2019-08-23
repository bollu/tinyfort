#include <math.h>
#include <iostream>
#include <assert.h>
#include <string>
#include <stdio.h>
#include "tinyfort/ir.h"
#include <cstdio>
void align(std::ostream &o, int depth) {
    for(int i = 0; i < depth; ++i) o << " ";
}


int compile_program(tf::Program *p) { 
    for(auto it : p->fndefns) {
        it->print(std::cout);
    }
    return 0;
}
