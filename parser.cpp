#include "parser.h"
#include "symbol.h"

#include <stdio.h>
#include <iostream>

extern table_stack table;
extern CodeComposer composer;

extern int yylineno;
using namespace std;




///******************************************FORMALS LIST***************************************************

FormalsList::FormalsList(FormalDecl* dec, FormalsList* list) : Node(dec->name){
    this->param_list.push_back(*dec);
    for( auto& item : list->param_list){
        this->param_list.push_back(item);
    }
}

FormalsList::FormalsList(FormalDecl *dec) : Node(dec->name) {
    this->param_list.push_back(*dec);
}

///******************************************EXP***************************************************

//exp -> exp binop/relop/and/or exp
Exp::Exp(Exp *e1, Node *op, Exp *e2, string markerLabel) : Node(op->name) {
    string type1 = e1->type;
    string type2 = e2->type;
    string operation = op->name;

    //exp -> exp binop exp
    if(operation == "+" || operation == "-" || operation == "*" || operation == "/") {
        if( (type1 != "int" && type1 != "byte") || (type2 != "int" && type2 != "byte") ) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if(type1 == "byte" && type2 == "byte") {
            this->type = "byte";
        }
        else {
            this->type = "int";
        }
        composer.composeAndEmitBinop(this, e1, operation, e2);
    }

    //exp -> exp relop exp
    if(operation == "<" || operation == ">" || operation == "<=" || operation == ">=" ||
       operation == "!=" || operation == "==") {
        if( (type1 != "int" && type1 != "byte") || (type2 != "int" && type2 != "byte") ) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->type = "bool";
        composer.composeAndEmitRelop(this, e1, operation, e2);
    }

    //exp -> exp AND||OR exp
    if(operation == "and" || operation == "or") {
        if(type1 != "bool" || type2 != "bool") {
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->type = "bool";
        composer.composeAndEmitOrAnd(this, e1, operation, e2, markerLabel);
    }
}

//exp -> not exp
Exp::Exp(Node *op, Exp *e) : Node(op->name) {
    if(e->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    this->type = "bool";
    composer.flipLists(this, e);
}

//exp -> num b
Exp::Exp(Node *n, Node *b) : Node(n->name) {
    int num = stoi(n->name);
    if (num > 255) {
        output::errorByteTooLarge(yylineno, n->name);
        exit(0);
    }
    this->type = "byte";
    composer.allocateAndEmitNumB(this);
}

//exp -> num/string/true/false
Exp::Exp(Node *n, std::string type) : Node(n->name), type(type) {
    if(type == "int") {
        composer.allocateAndEmitNum(this);
    }
    if(type == "string") {
        composer.allocateAndEmitString(this);
    }
    if(type == "bool") {
        composer.allocateAndEmitBool(this);
    }

}

Exp::Exp(Type *t, Exp *e) : Node(e->name) {
    string e_type = e->type;
    if((e_type != "byte" && e_type != "int") || (t->type != "byte" && t->type != "int")) {
        output::errorMismatch(yylineno);
        exit(0);
    }
    this->type = t->type;
    this->reg = e->reg;
}

//exp -> id
Exp::Exp(Node *id) : Node(id->name) {
    symbol_table_entry* entry = table.get_variable(id->name);
    this->type = entry->type;
    int offset = entry->offset;
    if(offset < 0) {
        composer.saveFuncArg(this, offset);
    } else {
        composer.loadVar(this, offset);
    }
}


void check_exp(Exp* exp) {
    if(exp->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }
}

///******************************************EXP LIST*******************************************

ExpList::ExpList(Exp *e) : Node(e->name) {
    if(e->type == "bool") {
        composer.boolValEval(e);
    }
    this->expressions.push_back(*e);
}

ExpList::ExpList(Exp *e, ExpList *list) : Node(e->name) {
    if(e->type == "bool") {
        composer.boolValEval(e);
    }
    this->expressions.push_back(*e);
    for( auto& item : list->expressions){
        this->expressions.push_back(item);
    }
}

///****************************************** STATEMENT *******************************************

Statement::Statement(Statements *sts) {
    this->next_list = sts->next_list;
    this->break_list = sts->break_list;
    this->continue_list = sts->continue_list;
}

//statement -> type ID ;
Statement::Statement(Type *t, Node *id) {
    table.insert_symbol(id->name, t->type);
    int offset = table.get_variable(id->name)->offset;
    Exp temp;
    temp.type = t->type;
    if(t->type == "bool") {
        temp.name = "false";
        composer.allocateAndEmitBool(&temp);
    } else {
        temp.name = "0";
        composer.allocateAndEmitNum(&temp);
        composer.storeVar(&temp, offset);
    }
}

//statement -> Type ID = Exp ;
Statement::Statement(Type *t, Node *id, Exp *e) {
    if(!type_compatible(t->type, e->type)) {
        output::errorMismatch(yylineno);
        exit(0);
    }
    table.insert_symbol(id->name, t->type);
    int offset = table.get_variable(id->name)->offset;
    if(t->type == "bool") {
        composer.boolValEval(e);
    }
    composer.storeVar(e, offset);
}

/*statement -> ID = Exp ;
/statement -> return exp;*/
Statement::Statement(Node *id, Exp *e) {
    if(id->name == "return"){
        /* string ret_type = table.tables_stack.back().func_ret_type; */
        string ret_type = table.get_closest_func_return_type();

        if(ret_type == "not_nested_in_func"){
            /*dont think this case is even reachable*/
            output::errorMismatch(yylineno);
            exit(0);
        }

        if(!(type_compatible(ret_type, e->type))) {
            output::errorMismatch(yylineno);
            exit(0);
        }

    }
    else{
        symbol_table_entry* entry = table.get_variable(id->name);
        int offset = entry->offset;
        //if we get here, symbol exists, otherwise we wouldve thrown error from the function

        if(!type_compatible(entry->type, e->type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        //this prod rule wont be used on function arguments - offset always >= 0
        if(offset >= 0) {
            if(e->type == "bool") {
                composer.boolValEval(e);
            }
            composer.storeVar(e, offset);
        }
    }
}



Statement::Statement(Node* n) {
    if(n->name == "return") {
        //string ret_type = table.tables_stack.back().func_ret_type;
        string ret_type = table.get_closest_func_return_type();
        if(ret_type == "not_nested_in_func"){
            /*dont think this case is even reachable*/
            output::errorMismatch(yylineno);
            exit(0);
        }

        
        //if(table.tables_stack.back().is_loop)
        //{
        /*TODO: not sure this check is the right thing to do:
        int foo(){
            while(true)
            {
            return;
            }
        }
        this return does not match the function, we want an error.
        */
        //    return;
        //}  
        


        if (ret_type != "void") {
            output::errorMismatch(yylineno);
            exit(0);
        }

    }

    if(n->name == "break") {
        if(!(table.checkLoop())) {
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }
    }
    if(n->name == "continue") {
        if(!(table.checkLoop())) {
            output::errorUnexpectedContinue(yylineno);
            exit(0);
        }
    }

}


//statement -> if (exp) statement
Statement::Statement(Exp* e, MarkerM* body, Statement* st) {
    if(e->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    break_list = st->break_list;
    continue_list = st->continue_list;
    buffer.bpatch(e->truelist, body->name);\
    string end_of_if = composer.new_label("end_of_if_label_");
    buffer.emit("br label %" + end_of_if);
    buffer.emit(end_of_if + ":");
    this->next_list = buffer.merge(st->next_list, e->nextlist);
    this->next_list = buffer.merge(this->next_list, e->falselist);
    buffer.bpatch(this->next_list, end_of_if);

}

//statement -> if (exp) statement else statement
Statement::Statement(Exp *e, MarkerM *if_body_marker, Statement *if_st, MarkerM *else_body_marker, Statement *else_st) {
    if(e->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }

    this->continue_list = buffer.merge(if_st->continue_list, else_st->continue_list);
    this->break_list = buffer.merge(if_st->break_list, else_st->break_list);
    buffer.bpatch(e->truelist, if_body_marker->name);
    buffer.bpatch(e->falselist, else_body_marker->name);
    string end_of_if_else = composer.new_label("end_of_if_else_label_");
    buffer.emit("br label %" + end_of_if_else);
    buffer.emit(end_of_if_else + ":");
    this->next_list = buffer.merge(if_st->next_list, else_st->next_list);
    this->next_list = buffer.merge(this->next_list, e->nextlist);
    buffer.bpatch(this->next_list, end_of_if_else);
}



///****************************************** Call *******************************************


Call::Call(Node *id, ExpList *params)  : Node(id->name) {
    vector<string> par = {};
    if(params){
        for(int i = 0; i < params->expressions.size(); i++){
            par.push_back(params->expressions[i].type);
        }
    }
    this->type = table.get_function(id->name, par)->type;
    /*errors(if there are any) are thrown from within get_functions*/
}


///******************************************FUNC DECL***************************************************


FuncDecl::FuncDecl(OverRide* override, RetType* rt, Node* id, Formals* params){
    vector<FormalDecl> param_list = params->param_list;
    vector<string> types{};
    vector<string> ids{};
    for(int i = 0; i<param_list.size(); i++) {
        types.push_back(param_list[i].type);
        ids.push_back(param_list[i].name);
    }
    table.insert_symbol(id->name, rt->type, true, override->isOverRide, types);
    table.insert_func_args(types, ids, rt->type);
    //buils vector params
}

//when marker is negzar, the lexeme in node is the label given to the marker.
MarkerM::MarkerM() {
    this->name = composer.new_label();
    buffer.emit("br label %" + name);
    buffer.emit(name + ":");
}