#include "symbol.h"
#include "hw3_output.hpp"
#include "iostream"

extern table_stack table;
extern int yylineno;

/*******************************HEALPERS**********************************************/

bool type_compatible(const string& right, const string& left){
    if (right != left) {
        if (!(right == "int" && left == "byte")) {
            return false;
        }
    }
    return true;
}


static bool type_vector_compatible(const vector<string>& right, const vector<string>& left){
    if(right.size() != left.size()){
        return false;
    }
    for(int i=0; i<right.size(); i++){
        if(!type_compatible(right[i], left[i])){
            return false;
        }
    }
    return true;
}


static bool are_vectors_equal(const vector<string>& a,const  vector<string>& b) {
    if(a.size() != b.size()) {
        return false;
    }
    for(int i = 0; i < a.size(); i++) {
        if(a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

///********************* SYMBOL TABLE SCOPE ********************************///

///upon inserting, check if exists, if yes, exit program
bool symbol_table_scope::exists_in_scope(const symbol_table_entry& entry) {
    for(auto it = entries.begin(); it != entries.end(); it++) {
        if(it->name == entry.name) {
            if(it->is_func == false || entry.is_func == false) {
                output::errorDef(yylineno, entry.name);
                exit(0);
            }
            //added: incase neither is override:
            if(it->is_override == false && entry.is_override == false) {
                output::errorDef(yylineno, entry.name);
                exit(0);
            }
            //if we reach this, both are funcs
            if(it->is_override == false) {
                output::errorFuncNoOverride(yylineno, it->name);
                exit(0);
            }
            if(entry.is_override == false) {
                output::errorOverrideWithoutDeclaration(yylineno, it->name);
                exit(0);
            }

            //if we reach this, we know that both are override
            if(it->type != entry.type){
                //piazza @92
                continue;
            }

            if(are_vectors_equal(it->params, entry.params)) {
                output::errorDef(yylineno, entry.name);
                exit(0);
            }
        }
    }
    return false;
}

///called from insert to table stack, and called after checking exists_in_scope to all scopes
void symbol_table_scope::insert_to_scope(symbol_table_entry &entry) {
    entries.push_back(entry);
}


void symbol_table_scope::print_scope() {
    for(auto it=(this->entries).begin(); it != (this->entries).end(); it++){
        if(it->is_func) {
            output::printID(it->name, 0, output::makeFunctionType(it->type, it->params));
        }
        else {
            output::printID(it->name, it->offset, it->type);
        }
    }
}

static bool are_params_equal(const vector<string>& called, const vector<string>& declared) {
    if(called.size() != declared.size())
        return false;

    for(int i = 0; i < called.size() ; i++){
        if(called[i] != declared[i]){
            if(!(called[i] == "byte" && declared[i] == "int")) {
                return false;
            }
        }
    }
    return true;
}

//this func is only for variables (id)
symbol_table_entry* symbol_table_scope::get_variable_in_scope(const string &name) {
    for(auto it = entries.begin(); it != entries.end(); it++) {
        if(name == it->name && it->is_func == false)
            return &(*it);
    }
    return nullptr;
}


/*bool symbol_table_scope::symbol_declared_in_scope(const symbol_table_entry &entry) {
    if(entry.is_func == false) {
        for(auto it = entries.begin(); it != entries.end(); it++) {
            if(entry.name == it->name && it->is_func == false)
                return true;
        }
    }
    bool found_name = false;
    int counter = 0;
    if(entry.is_func == true) {
        for(auto it = entries.begin(); it != entries.end(); it++) {
                if(entry.name == it->name) {
                    found_name = true;
                    bool answer = are_params_equal(entry.params, it->params);
                    if(answer == true)
                        counter++;
                }
        }
        if(counter > 1) {
            output::errorAmbiguousCall(yylineno, entry.name);
            exit(0);
        }
        if(counter == 1)
            return true;
        if(counter == 0 && found_name == true){
            output::errorPrototypeMismatch(yylineno, entry.name);
            exit(0);
        }
    }
    return false;
}*/




///********************* TABLE STACK ********************************///
table_stack::table_stack(){
    this->offsets_stack.push(0);
    this->open_scope();
    this->insert_symbol("print","void",true,false,{"string"});
    this->insert_symbol("printi","void",true,false,{"int"});
}

void table_stack::final_check() {
    symbol_table_entry* main_enrty = get_function("main");
    if(main_enrty->type != "void" || main_enrty->params.size() > 0){
        output::errorMainMissing();
        exit(0);
    }
    this->close_scope();
}

bool table_stack::symbol_exists(const symbol_table_entry& entry) {
    for(auto it = tables_stack.begin(); it != tables_stack.end(); it++) {
        it->exists_in_scope(entry);
    }
    return true;
}

void table_stack::insert_symbol(const string &n, string t, bool func, bool override, vector <string> p) {
    if(DEBUG){
        cout << "in insert_symbol with: " << t << ", " << n << ", is func: " <<  func << ", is override: " << override << endl; 
        cout << endl;
    }

    if (n == "main" && override == true) {
        output::errorMainOverride(yylineno);
        exit(0);
    }
    int insert_offset = func ? 0 : offsets_stack.top();
    string uniqe_name;
    if( func && override ) {
        uniqe_name = n + to_string(this->override_counter);
        this->override_counter++;
    }
    else {
        uniqe_name = n;
    }
    symbol_table_entry entry(n,uniqe_name, t, insert_offset, func, override, p);

    this->symbol_exists(entry);

    //if we got here, the insertion is legal
    tables_stack.back().insert_to_scope(entry);

    if (!func) {
        offsets_stack.pop();
        offsets_stack.push(insert_offset + 1);
    }
}


void table_stack::insert_func_args(vector<string> types, vector<string> names, string retType) {
    if(DEBUG){
        cout << "in insert_func_args" << tables_stack.size() << endl;
    }

    this->open_scope(false, retType);


    int offset = -1;
    for(int i = 0; i < types.size(); i++) {
        symbol_table_entry entry(names[i], names[i], types[i], offset);

        /*checking that we dont insert the same parameter like - foo(int x, int x)*/
        symbol_exists(entry);

        this->tables_stack.back().entries.push_back(entry);

        offset--;
    }
}


void table_stack::open_scope(bool is_loop, string ret_type)
{
    if(DEBUG){
        cout << "opening a new scope, number of scopes before is: " << tables_stack.size() << endl;
        cout << "max offset before is: " << offsets_stack.top() << endl;
    }
    
    symbol_table_scope new_scope(is_loop, ret_type);
    tables_stack.push_back(new_scope);
    offsets_stack.push(offsets_stack.top());

    if(DEBUG){
        cout << "number of scopes after is: " << tables_stack.size() << endl;
        cout << "max offset after is: " << offsets_stack.top() << endl;
        cout << endl;
    }

}

void table_stack::close_scope() {
    if(DEBUG){
        cout << "going to close scope, number of scopes before is: " << tables_stack.size() << endl;
        cout << "max offset before is: " << offsets_stack.top() << endl;
    }
    //output::endScope();
    symbol_table_scope to_close = tables_stack.back();
    //to_close.print_scope();
    tables_stack.pop_back();
    offsets_stack.pop();

    if(DEBUG){
        cout << "number of scopes after is: " << tables_stack.size() << endl;
        if( offsets_stack.empty() ){
            cout << "offset stack is empty" << endl;
        }
        else{
            cout << "max offset after is: " << offsets_stack.top() << endl;
        }
        cout << endl;
    }
}

symbol_table_entry* table_stack::get_variable(const string &name) {
    for(auto it = this->tables_stack.begin(); it != this->tables_stack.end();  it++) {
        symbol_table_entry *returned = it->get_variable_in_scope(name);
        if (returned != nullptr) {
            return returned;
        }
    }
    output::errorUndef(yylineno, name);
    exit(0);
}


symbol_table_entry* table_stack::get_function(const string& name, vector<string> params){
    bool found_name = false;
    int counter = 0;
    symbol_table_entry* entry = nullptr;
    for(auto it = (this->tables_stack[0].entries).begin() ; it != (this->tables_stack[0].entries).end(); it++){
        if(name == it->name){
            found_name = true;
            
            if(type_vector_compatible(it->params, params)){
                counter++;
                entry = &(*it);
            }
        }
    }

    if(counter == 1){
        return entry;
    }

    if(counter > 1){
        /*we found more than one function with the same name and args are compatible*/
        output::errorAmbiguousCall(yylineno, name);
        exit(0);
    }

    if(found_name && name != "main"){
        /*counter == 0 but name was found, meaning there is a function under that name but with uncompaatible args.
        added name != "main" to deal with test 15*/
        output::errorPrototypeMismatch(yylineno, name);
        exit(0);
    }

    if(name == "main"){
        /*counter == 0  and found name == false*/
        output::errorMainMissing();
        exit(0);
    }

    /*if we get here, there is no function under wanted name */
    output::errorUndefFunc(yylineno, name);
    exit(0);    
}

/*returns true if scope is nested inside a loop scope, false otherwise
unsing rbegin, we itarate from upp to down*/
bool table_stack::checkLoop() {
    for(auto it = this->tables_stack.rbegin(); it != this->tables_stack.rend(); it++) {
        if(it->is_loop)
        {
            return true;
        }
    }
    return false;
}


/*if a scope is nested inside a function scope, will return its return type
return values: 
"not_nested_in_func" - if it is not nested in func scope
type otherwise "*/
string table_stack::get_closest_func_return_type(){
    for(auto it = this->tables_stack.rbegin(); it != this->tables_stack.rend(); it++) {
        if(it->func_ret_type != ""){
            return it->func_ret_type;
        }
    }
    return "not_nested_in_func";
}


/*
this function was not used

///in inserting, we made sure that a varibale appears only once, so its enough for use to check if a variable exists in one scope
///havent used for variable. might use for functions
bool table_stack::symbol_declared(const symbol_table_entry &entry) {
    for(auto it = this->tables_stack.begin(); it != this->tables_stack.end(); it++) {
        bool answer = it->symbol_declared_in_scope(entry);
        if(answer == true)
            return true;
    }
    if(entry.is_func == false) {
        output::errorUndef(yylineno, entry.name);
        exit(0);
    }
    if(entry.is_func) {
        output::errorUndefFunc(yylineno, entry.name);
    }
    return false;
}
*/