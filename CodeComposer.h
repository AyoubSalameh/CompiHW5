#ifndef COMPIHW5_COMPOSER_H
#define COMPIHW5_COMPOSER_H
#include <string>
#include "bp.hpp"
#include "defs.h"
#include "parser.h"

extern CodeComposer composer;
extern CodeBuffer buffer;

using namespace std;

class CodeComposer{
private:
    long cur_register;
    long cur_label;

public:
    int max_num_of_vars_per_func = 50;
    string top_function_rbp = "";

    CodeComposer(): cur_register(0), cur_label(0) {}
    static CodeComposer &instance();

    string new_register();
    string new_global_register();
    string new_label(string label = "label_");
    //void emitGlobals();

    void boolValEval(Exp* exp);
    void emitBranchNext(Exp* exp);

    void allocateAndEmitNumB(Exp* exp);
    void allocateAndEmitNum(Exp* exp);
    void allocateAndEmitString(Exp* exp);
    void allocateAndEmitBool(Exp* exp);
    void composeAndEmitBinop(Exp* lhs, Exp* exp1, string op, Exp* exp2);
    void composeAndEmitRelop(Exp* lhs, Exp* exp1, string op, Exp* exp2);
    void composeAndEmitOrAnd(Exp* lhs, Exp* exp1, string op, Exp* exp2, string marker);
    void composeAndEmitFuncDecl(RetType *ret_type, string uniqe_func_name, Formals *params);
    void composeAndEmitEndFunc(RetType *ret_type, Statements* sts);
    void composeAndEmitCall(Call* func, string unique_name ,ExpList* args);
    void flipLists(Exp* left, Exp* right);
    void saveFuncArg(Exp* exp, int offset);
    void loadVar(Exp* exp, int offset);
    void storeVar(Exp* exp, int offset);
    string allocaFunctionStack();

};

#endif //COMPIHW5_COMPOSER_H
