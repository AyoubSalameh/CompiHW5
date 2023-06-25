#include "CodeComposer.h"

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
    string true_label = new_label("PHI_TRUE_LABEL_");
    string false_label = new_label("PHI_FALSE_LABEL_");
    string next_label = new_label("PHI_NEXT_LABEL_");
    buffer.emit(true_label + ":");
    int true_address = buffer.emit("br label @");
    //TODO: instead of @ and bp, try to emit the next label directly
    buffer.emit(false_label + ":");
    int false_address = buffer.emit("br label @");
    //TODO: instead of @ and bp, try to emit the next label directly
    buffer.emit(next_label + ":");

    if(exp->reg == "")
    {
        exp->reg = new_register();
    }
    buffer.emit(exp->reg + " = phi i32 [1, %" + true_label + "], [0, %" + false_label + "]");

    bplist next = buffer.merge(buffer.makelist(bplist_pair(true_address, FIRST)),
                               buffer.makelist(bplist_pair(false_address, FIRST)));
    buffer.bpatch(exp->truelist, true_label);
    buffer.bpatch(exp->falselist, false_label);
    buffer.bpatch(next, next_label);
}

void CodeComposer::allocateAndEmitNumB(Exp *exp) {
    exp->reg = this->new_register();
    buffer.emit(exp->reg + " = add i32 " + exp->name + ", 0");
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
    buffer.emitGlobal(temp_reg + "constant" + arrSize + " c" + str + "\\00\"");

    //    %format_ptr = getelementptr [4 x i8], [4 x i8]* @.intFormat, i32 0, i32 0
    string get_reg = "getelementptr" + arrSize + ", " + arrSize + "* " + temp_reg + ", i32 0, i32 0" ;
    str.replace(0, 1, "%"); //%ayoub
    buffer.emit(str + "_ptr = " + get_reg);
    exp->reg = temp_reg + "_ptr";
}

void CodeComposer::allocateAndEmitBool(Exp *exp) {
    int address = buffer.emit("br label @");
    if(exp->name == "true") {
        exp->truelist = buffer.makelist(bplist_pair(address, FIRST));
    } else {
        exp->falselist = buffer.makelist(bplist_pair(address, SECOND));
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
            op_cmd == "udiv";
        } else {
            op_cmd == "sdiv";
        }
        buffer.emit("call void @divide_by_zero_check(i32 " + exp2->reg +")");
    }
    buffer.emit(lhs->reg + " = " + op_cmd + " i32 " + exp1->reg + ", " + exp2->reg);
    //TODO might need a different emit for byte
    //this code is for numeric surfing
    //TODO check if we can use trunc by asking
    if(lhs->type == "byte"){
        string orig = lhs->reg;
        lhs->reg = new_register();
        buffer.emit(lhs->reg + " = trunc i32 " + orig + " to i8");
    }
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

    buffer.emit(lhs->reg + " = icmp " + op_cmd + " i32 " + exp1->reg + ", " + exp2->reg);
    int hole_address = buffer.emit("br i1 " + lhs->reg + ", label @, label @");
    lhs->truelist = buffer.makelist(pair<int,BranchLabelIndex>(hole_address, FIRST));
    lhs->falselist = buffer.makelist(pair<int,BranchLabelIndex>(hole_address, SECOND));
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

void CodeComposer::flipLists(Exp *left, Exp *right) {
    //TODO: might need to add bplist(right->list)
    left->truelist = right->falselist;
    left->falselist = right->truelist;
}

void CodeComposer::saveFuncArg(Exp *exp, int offset) {
    string func_var = to_string( -1 * (offset + 1));
    exp->reg = "%" + func_var;
}

void CodeComposer::loadVar(Exp *exp, int offset) {
    string reg = new_register();
    string address_ptr = new_register();
    buffer.emit(address_ptr  + " = getelementptr i32, i32* " + this->top_function_rbp + ", i32 " + to_string(offset));
    buffer.emit(reg + " = load i32, i32* " + address_ptr);
    exp->reg = reg;
}

void CodeComposer::storeVar(Exp *exp, int offset) {
    string address_ptr = new_register();
    buffer.emit(address_ptr  + " = getelementptr i32, i32* " + this->top_function_rbp + ", i32 " + to_string(offset));
    buffer.emit("store i32 " + exp->reg + ", i32* " + address_ptr);

}