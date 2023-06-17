#ifndef COMPIHW5_COMPOSER_H
#define COMPIHW5_COMPOSER_H
#include <string>
#include "bp.hpp"
#include "parser.h"

extern CodeBuffer buffer;

using namespace std;

class CodeComposer{
private:
    long cur_register;
    long cur_label;

public:
    CodeComposer(): cur_register(0), cur_label(0) {}

    string new_register();
    string new_global_register();
    string new_label();
    //void emitGlobals();
    void allocateAndEmitNum(Exp* exp);
    void allocateAndEmitString(Exp* exp);

};

#endif //COMPIHW5_COMPOSER_H
