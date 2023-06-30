#include "CodeComposer.h"
#include "map"
#include <algorithm>

extern CodeComposer composer;
extern CodeBuffer buffer;

CodeComposer &CodeComposer::instance() {
	static CodeComposer inst;//only instance
	return inst;
}

string CodeComposer::new_register() {
    string reg = "%var" + to_string(cur_register);
    cur_register++;
    return reg;
}

string CodeComposer::new_num_of_register() {
    string reg = to_string(cur_register);
    cur_register++;
    return reg;
}

string CodeComposer::new_global_register() {
    string reg = "@var" + to_string(cur_register);
    cur_register++;
    return reg;
}

string CodeComposer::new_label(string label) {
    string new_label = label + to_string(cur_label);
    cur_label++;
    return new_label;
}

/*this function is used to emit code that uses phi to evaluate bool expressions to 0 or 1 */
void CodeComposer::boolValEval(Exp *exp) {

    exp->reg = new_register();

    string true_label = new_label("PHI_TRUE_LABEL_");
    string false_label = new_label("PHI_FALSE_LABEL_");
    string next_label = new_label("PHI_NEXT_LABEL_");

    buffer.emit("br label %" + true_label);
    //we think this line is nedded for basic blocks stuffs. gives an error without it.

    buffer.emit(true_label + ":");
    int true_address = buffer.emit("br label @");

    //TODO: instead of @ and bp, try to emit the next label directly
    buffer.emit(false_label + ":");
    int false_address = buffer.emit("br label @");
    //TODO: instead of @ and bp, try to emit the next label directly
    buffer.emit(next_label + ":");

    bplist next = buffer.merge(buffer.makelist(bplist_pair(true_address, FIRST)),
                               buffer.makelist(bplist_pair(false_address, FIRST)));
    buffer.bpatch(exp->truelist, true_label);
    buffer.bpatch(exp->falselist, false_label);
    buffer.bpatch(next, next_label);

    buffer.emit(exp->reg + " = phi i1 [ 1, %" + true_label + "], [0, %" + false_label + "]");
}

void CodeComposer::emitBranchNext(Exp* exp){
    int address = buffer.emit("br label @");
    exp->nextlist = buffer.merge(buffer.makelist(bplist_pair(address, FIRST)), exp->nextlist);
}

void CodeComposer::allocateAndEmitNumB(Exp *exp) {
    exp->reg = this->new_register();
    buffer.emit(exp->reg + " = add i8 " + exp->name + ", 0");
}

void CodeComposer::allocateAndEmitNum(Exp *exp) {
    exp->reg = this->new_register();
    buffer.emit(exp->reg + " = add i32 " + exp->name + ", 0");
}

void CodeComposer::allocateAndEmitString(Exp *exp) {
    string temp_reg = this->new_global_register();
    string str = exp->name;
    str.pop_back();         //"ayoub" -> "ayoub
    string arrSize = "[" + to_string(str.size()) + " x i8]";
    buffer.emitGlobal(temp_reg + " = constant" + arrSize + " c" + str + "\\00\"");

    //    %format_ptr = getelementptr [4 x i8], [4 x i8]* @.intFormat, i32 0, i32 0
    string get_reg = "getelementptr" + arrSize + ", " + arrSize + "* " + temp_reg + ", i32 0, i32 0" ;

    string new_reg = new_register();
    buffer.emit(new_reg + " = " + get_reg);
    exp->reg = new_reg;
    
}

void CodeComposer::allocateAndEmitBool(Exp *exp) {
    int address = buffer.emit("br label @");
    if(exp->name == "true") {
        exp->truelist = buffer.makelist(bplist_pair(address, FIRST));
    } else {
        exp->falselist = buffer.makelist(bplist_pair(address, FIRST));
    }
}

void CodeComposer::composeAndEmitBinop(Exp *lhs, Exp *exp1, string op, Exp *exp2) {
    lhs->reg = new_register();
    string op_cmd;
    if (op == "+") op_cmd = "add";
    if (op == "-") op_cmd = "sub";
    if (op == "*") op_cmd = "mul";
    if (op == "/"){
        if(lhs->type == "byte") {
            op_cmd = "udiv";
        } else {
            op_cmd = "sdiv";
        }

        if(exp2->type == "byte"){
            string converted_reg_for_check_zero = new_register();
            buffer.emit(converted_reg_for_check_zero + " = zext i8 " + exp2->reg + " to i32");
            buffer.emit("call void @divide_by_zero_check(i32 " + converted_reg_for_check_zero +")");
        }
        else{
            buffer.emit("call void @divide_by_zero_check(i32 " + exp2->reg +")");
        }
    }
    if( lhs->type == "byte"){
        /*this means that both exp1, and exp2 are bytes.
        llvm is spposed to handle numeric overflow on its own (at least thats what gabeta says) */
        buffer.emit(lhs->reg + " = " + op_cmd + " i8 " + exp1->reg + ", " + exp2->reg);
    }
    else{
        if(exp1->type == "int" && exp2->type == "int"){
            buffer.emit(lhs->reg + " = " + op_cmd + " i32 " + exp1->reg + ", " + exp2->reg);
        }
        else{
            /*one is int and the other is byte, we want to convert the byte to int and use it instead.
            %result = zext i8 %byte to i32.*/
            string reg_to_convert = (exp1->type == "byte") ? exp1->reg : exp2->reg;
            string the_int_one = (exp1->type == "byte") ? exp2->reg : exp1->reg;

            string converted_reg = new_register();
            buffer.emit(converted_reg + " = zext i8 " + reg_to_convert + " to i32");
            
            buffer.emit(lhs->reg + " = " + op_cmd + " i32 " + converted_reg + ", " + the_int_one);
        }
    }
    
    /*
    //TODO might need a different emit for byte
    //this code is for numeric surfing
    //TODO check if we can use trunc by asking
    if(lhs->type == "byte"){
        string orig = lhs->reg;
        lhs->reg = new_register();
        buffer.emit(lhs->reg + " = trunc i32 " + orig + " to i8");
    }*/
}

void CodeComposer::composeAndEmitRelop(Exp *lhs, Exp *exp1, string op, Exp *exp2) {
    lhs->reg = new_register();
    string op_cmd;
    if (op == "<") op_cmd = "slt";
    if (op == ">") op_cmd = "sgt";
    if (op == "<=") op_cmd = "sle";
    if (op == ">=") op_cmd = "sge";
    if (op == "!=") op_cmd = "ne";
    if (op == "==") op_cmd = "eq";

    if(exp1->type == "int" && exp2->type == "int") {
        buffer.emit(lhs->reg + " = icmp " + op_cmd + " i32 " + exp1->reg + ", " + exp2->reg);
    }
    else if(exp1->type == "byte" && exp2->type == "byte") {
        buffer.emit(lhs->reg + " = icmp " + op_cmd + " i8 " + exp1->reg + ", " + exp2->reg);
    }
    else {
        string reg_to_convert = (exp1->type == "byte") ? exp1->reg : exp2->reg;
        string the_int_one = (exp1->type == "byte") ? exp2->reg : exp1->reg;
        string converted_reg = new_register();
        buffer.emit(converted_reg + " = zext i8 " + reg_to_convert + " to i32");

        buffer.emit(lhs->reg + " = icmp " + op_cmd + " i32 " + converted_reg + ", " + the_int_one);
    }

    int hole_address = buffer.emit("br i1 " + lhs->reg + ", label @, label @");
    lhs->truelist = buffer.makelist(pair<int, BranchLabelIndex>(hole_address, FIRST));
    lhs->falselist = buffer.makelist(pair<int, BranchLabelIndex>(hole_address, SECOND));
}

void CodeComposer::composeAndEmitOrAnd(Exp *lhs, Exp *exp1, string op, Exp *exp2, string marker) {
    if(op == "and") {
        buffer.bpatch(exp1->truelist, marker);
        lhs->truelist = exp2->truelist;
        lhs->falselist = buffer.merge(exp1->falselist, exp2->falselist);
    } else { //op == or
        buffer.bpatch(exp1->falselist, marker);
        lhs->falselist = exp2->falselist;
        lhs->truelist = buffer.merge(exp1->truelist, exp2->truelist);
    }
}

void CodeComposer::composeAndEmitFuncDecl(RetType *ret_type, string uniqe_func_name, Formals *params) {
    /*not sure if everything shuld be string or i32*/
    map<string, string> typesMap = {
    {"int", "i32"},
    {"byte", "i8" },
    {"bool", "i1"},
    {"void" , "void"},
    {"string", "i8*"}};

    string args_list_to_emit = "";
    string ret_type_to_emit = typesMap[ret_type->type];

    //preparing the line that has the types of all args
    for(int i=0; i < params->param_list.size(); i++) {
        string curr_type = params->param_list[i].type;
        args_list_to_emit += typesMap[curr_type];
        if(i != params->param_list.size()-1 ) {
            args_list_to_emit += ", ";
        }
    }

    buffer.emit("define " + ret_type_to_emit + " @" + uniqe_func_name + "(" + args_list_to_emit + "){");
    //the closing "}" in emmited in composeAndEmitEndFunc
}

void CodeComposer::composeAndEmitEndFunc(RetType *ret_type, Statements* sts) {
    //TODO:  complete - if we have time we should save a list<statement>n statements,
    //and check whether the last one is a return statment.
     map<string, string> retTypesMap = {
        {"int", "i32 0"},
        {"byte", "i8 0" },
        {"bool", "i1 0"},
        {"void", "void"}};

    buffer.emit("ret " + retTypesMap[ret_type->type]);
    buffer.emit("}");
}

void CodeComposer::composeAndEmitCall(Call* func, string unique_name ,ExpList* args, vector<string>& func_params) {
    func->reg = new_register();
    /*not sure if everything shuld be string or i32*/
    map<string, string> typesMap = {
            {"int", "i32"},
            {"byte", "i8" },
            {"bool", "i1"},
            {"string", "i8*"},
            {"void", "void"}};

    string args_list_to_emit = "";
    string ret_type_to_emit = typesMap[func->type];

    if(args) {
        //preparing the line that has the types of all args and their regs
        for (int i = 0; i < args->expressions.size(); i++) {
            string curr_arg_type = args->expressions[i].type;
            string curr_func_param_type = func_params[i];
            if( curr_arg_type == curr_func_param_type) {
                args_list_to_emit += typesMap[curr_arg_type];
                args_list_to_emit += " ";
                args_list_to_emit += args->expressions[i].reg;
            }
            else if( curr_func_param_type == "int" && curr_arg_type == "byte") {
                string converted_reg = new_register();
                buffer.emit(converted_reg + " = zext i8 " + args->expressions[i].reg + " to i32");
                args_list_to_emit += typesMap[curr_func_param_type];
                args_list_to_emit += " ";
                args_list_to_emit += converted_reg;
            }
            else if( curr_func_param_type == "byte" && curr_arg_type == "int" ) {
                string converted_reg = new_register();
                buffer.emit(converted_reg + " = trunc i32 " + args->expressions[i].reg + " to i8");
                args_list_to_emit += typesMap[curr_func_param_type];
                args_list_to_emit += " ";
                args_list_to_emit += converted_reg;
            }

            if (i != args->expressions.size() - 1) {
                args_list_to_emit += ", ";
            }
        }
    }
    
    string prefix = (func->type == "void") ? "" : ((func->reg) + " = ");

    buffer.emit(prefix + "call " + ret_type_to_emit + " @" + unique_name + "(" + args_list_to_emit + ")");

}

void CodeComposer::flipLists(Exp *left, Exp *right) {
    //TODO: might need to add bplist(right->list)
    left->truelist = right->falselist;
    left->falselist = right->truelist;
}

void CodeComposer::saveFuncArg(Exp *exp, int offset) {
    buffer.emit(";**** saving function arg");
    string func_var = to_string( -1 * (offset + 1));
    exp->reg = "%" + func_var;
    buffer.emit("; " + exp->reg);
}

void CodeComposer::loadVar(Exp *exp, int offset) {
    /*dont know how to support diffrent types on the stack, so the stack only handle i32
    and we convert it if needed.*/
    string reg = new_register();
    string address_ptr = new_register();
    buffer.emit(address_ptr  + " = getelementptr i32, i32* " + this->top_function_rbp + ", i32 " + to_string(offset));
    buffer.emit(reg + " = load i32, i32* " + address_ptr);
    if(exp->type == "byte") {
        string convetred_reg = new_register();
        buffer.emit(convetred_reg + " = trunc i32 " + reg + " to i8");
        exp->reg = convetred_reg;
    }
    else if(exp->type == "bool") {
        string convetred_reg = new_register();
        buffer.emit(convetred_reg + " = trunc i32 " + reg + " to i1");
        exp->reg = convetred_reg;
    }
    else{
        exp->reg = reg;
    }
    
}

void CodeComposer::storeVar(Exp *exp, int offset) {
    /*dont know how to support diffrent types on the stack, so the stack only handle i32
    and we convert it if needed.*/
    string address_ptr = new_register();
    buffer.emit(address_ptr  + " = getelementptr i32, i32* " + this->top_function_rbp + ", i32 " + to_string(offset));
    if(exp->type == "byte") {
        string convetred_reg = new_register();
        buffer.emit(convetred_reg + " = zext i8 " + exp->reg + " to i32");
        buffer.emit("store i32 " + convetred_reg + ", i32* " + address_ptr);
    }
    else if(exp->type == "bool"){
        string convetred_reg = new_register();
        buffer.emit(convetred_reg + " = zext i1 " + exp->reg + " to i32");
        buffer.emit("store i32 " + convetred_reg + ", i32* " + address_ptr);
    }
    else{
        buffer.emit("store i32 " + exp->reg + ", i32* " + address_ptr);
    }
}

string CodeComposer::allocaFunctionStack() {
    string frame_base = new_register();
    buffer.emit(frame_base + " = alloca i32, i32 " + to_string(this->max_num_of_vars_per_func));
    return frame_base;
}
