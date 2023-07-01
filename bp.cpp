#include "bp.hpp"
#include <vector>
#include <iostream>
#include <sstream>
using namespace std;

bool replace(string& str, const string& from, const string& to, const BranchLabelIndex index);

CodeBuffer::CodeBuffer() : buffer(), globalDefs() {
    //might need to add internal constant and zero_div.str
    this->emit("@.DIVISION_ERROR = internal constant[23 x i8] c\"Error division by zero\\00\"");
    this->emit("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
    this->emit("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
    this->emit("declare i32 @printf(i8*, ...)");
    this->emit("declare void @exit(i32)");

    //printi
    this->emit("define void @printi(i32) {");
    this->emit("%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0");
    this->emit("call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)");
    this->emit("ret void");
    this->emit("}");

    //print
    this->emit("define void @print(i8*) {");
    this->emit("%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0");
    this->emit("call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)");
    this->emit("ret void");
    this->emit("}");

    //function to check division by zero
    this->emit("define void @divide_by_zero_check(i32) {");
    this->emit("%result = icmp eq i32 %0 ,0");
    this->emit("br i1 %result, label %divided_by_zero, label %divided_by_zero_end");
    this->emit("divided_by_zero:");
    this->emit("%ptr_div_by_zero = getelementptr [23 x i8], [23 x i8]* @.DIVISION_ERROR, i32 0, i32 0");
    this->emit("call void @print(i8* %ptr_div_by_zero)");
    this->emit("call void @exit(i32 0)");
    this->emit("ret void");
    this->emit("divided_by_zero_end:");
    this->emit("ret void");
    this->emit("}");
}

CodeBuffer &CodeBuffer::instance() {
	static CodeBuffer inst;//only instance
	return inst;
}

string CodeBuffer::genLabel(){
	std::stringstream label;
	label << "label_";
	label << buffer.size();
	std::string ret(label.str());
	label << ":";
	emit(label.str());
	return ret;
}

int CodeBuffer::emit(const string &s){
    buffer.push_back(s);
	return buffer.size() - 1;
}

void CodeBuffer::bpatch(const vector<pair<int,BranchLabelIndex>>& address_list, const std::string &label){
    for(vector<pair<int,BranchLabelIndex>>::const_iterator i = address_list.begin(); i != address_list.end(); i++){
    	int address = (*i).first;
    	BranchLabelIndex labelIndex = (*i).second;
		replace(buffer[address], "@", "%" + label, labelIndex);
    }
}

void CodeBuffer::printCodeBuffer(){
	for (std::vector<string>::const_iterator it = buffer.begin(); it != buffer.end(); ++it) 
	{
		cout << *it << endl;
    }
}

vector<pair<int,BranchLabelIndex>> CodeBuffer::makelist(pair<int,BranchLabelIndex> item)
{
	vector<pair<int,BranchLabelIndex>> newList;
	newList.push_back(item);
	return newList;
}

vector<pair<int,BranchLabelIndex>> CodeBuffer::merge(const vector<pair<int,BranchLabelIndex>> &l1,const vector<pair<int,BranchLabelIndex>> &l2)
{
	vector<pair<int,BranchLabelIndex>> newList(l1.begin(),l1.end());
	newList.insert(newList.end(),l2.begin(),l2.end());
	return newList;
}

// ******** Methods to handle the global section ********** //
void CodeBuffer::emitGlobal(const std::string& dataLine) 
{
	globalDefs.push_back(dataLine);
}

void CodeBuffer::printGlobalBuffer()
{
	for (vector<string>::const_iterator it = globalDefs.begin(); it != globalDefs.end(); ++it)
	{
		cout << *it << endl;
	}
}

// ******** Helper Methods ********** //
bool replace(string& str, const string& from, const string& to, const BranchLabelIndex index) {
	size_t pos;
	if (index == SECOND) {
		pos = str.find_last_of(from);
	}
	else { //FIRST
		pos = str.find_first_of(from);
	}
    if (pos == string::npos)
        return false;
    str.replace(pos, from.length(), to);
    return true;
}


std::string CodeBuffer::getLastEmittedLine() {
    return(this->buffer.back());
}