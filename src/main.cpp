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

void tf::printBinop(std::ostream &o, tf::Binop bp) {
    switch(bp) {
        case tf::BinopAdd: o << "+"; return;
        case tf::BinopSub: o << "-"; return;
        case tf::BinopMul: o << "*"; return;
        case tf::BinopDiv: o << "/"; return;
        case tf::BinopOr: o << "||"; return;
        case tf::BinopLeq: o << "<="; return;
        case tf::BinopGeq: o << ">="; return;
        default: assert(false && "unreachable");
    }
}


int compile_program(tf::Program *p) { 
    for(auto it : p->fndefns) {
        it->print(std::cout);
    }
    return 0;
}
