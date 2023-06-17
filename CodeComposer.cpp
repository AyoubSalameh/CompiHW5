#include "CodeComposer.h"

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

string CodeComposer::new_label() {
    string label = "label_" + to_string(cur_label);
    cur_label++;
    return label;
}

void CodeComposer::allocateAndEmitNum(Exp *exp) {
    exp->reg = this->new_register()
    buffer.emit(exp->reg + " = add i32 " + exp->name + ", 0");
}

void CodeComposer::allocateAndEmitString(Exp *exp) {
    string temp_reg = this->new_global_register();
    string str = exp->name;
    str.pop_back();         //"ayoub" -> "ayoub
    string arrSize = "[" + to_string(str.size()) + " x i8]";
    buffer.emitGlobal(reg + "constant" + arrSize " c" + str + "\\00\"");

    //    %format_ptr = getelementptr [4 x i8], [4 x i8]* @.intFormat, i32 0, i32 0
    string get_reg = "getelementptr" + arrSize + ", " + arrSize + "* " + temp_reg ", i32 0, i32 0";
    str.replace(0, 1, "%"); //%ayoub
    buffer.emit(str + "_ptr = " + get_reg);
    exp->reg = reg + "_ptr";
}

/*
void CodeComposer::emitGlobals() {
    //might need to add internal constant and zero_div.str
    buffer.emit("@.DIVISION_ERROR = internal constant[23 x i8] c\"Error division by zero\\00\"");
    buffer.emit("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
    buffer.emit("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
    buffer.emit("declare i32 @printf(i8*, ...)");
    buffer.emit("declare void @exit(i32)");

    //printi
    buffer.emit("define void @printi(i32) {");
    buffer.emit("%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0");
    buffer.emit("call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)");
    buffer.emit("ret void");
    buffer.emit("}");

    //print
    buffer.emit("define void @print(i8*) {");
    buffer.emit("%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0");
    buffer.emit("call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)");
    buffer.emit("ret void");
    buffer.emit("}");

    //function to check division by zero
    buffer.emit("define void @divide_by_zero_check(i32) {");
    buffer.emit("%result = icmp eq i32 %0 ,0");
    buffer.emit("br i1 %result, label %divided_by_zero, label %end");
    buffer.emit("divided_by_zero:");
    buffer.emit("%ptr = getelementptr [23 x i8], [23 x i8]* @.DIVISION_ERROR, i32 0, i32 0");
    buffer.emit("call i32 (i8*, ...) @print(i8* %ptr)");
    buffer.emit("call void @exit(i32 0)");
    buffer.emit("end:");
    buffer.emit("ret void");
    buffer.emit("}");
}*/
