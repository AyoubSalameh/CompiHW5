#ifndef HW3_OUTPUT_CPP_PARSER_H
#define HW3_OUTPUT_CPP_PARSER_H
#include <string>
#include <vector>
#include "symbol.h"
#include "hw3_output.hpp"
#include "bp.hpp"
#include "defs.h"
#include "CodeComposer.h"


/*class Node;
class Statements;
class Statement;
class Type;
class Call;
class Exp;
class ExpList;
class Statement;
class Statements;
class Program;
class Funcs;
class OverRide;
class RetType;
class FormalDecl;
class FormalsList;
class Formals;
class FuncDecl;*/


#define YYSTYPE Node*

extern char* yytext;


class Node {
public:
    std::string name;

    Node(const std::string s= ""): name(s) {}
    //Node(const Node* node) : name(node->name) {}
    virtual ~Node(){}
};

class Type : public Node {
public:
    std::string type;

    /*Type -> INT | BYTE | BOOL */
    Type(std::string t) : type(t) {}

    ~Type() = default;
};

class Call : public Node {
public:
    std::string type;
    std::string reg = "";
    bplist truelist = {};
    bplist falselist = {};
    bplist nextlist = {};

    /*Call -> ID (ExpList)
    Call -> ID ()*/
    Call(Node* id, ExpList* params = nullptr);

    ~Call() = default;
};

class Exp: public Node {
public:
    std::string type;
    std::string reg = "";
    bplist truelist = {};
    bplist falselist = {};
    bplist nextlist = {};

    //not for production rules. only for inner use
    Exp() : Node(), type("void") {}

    //exp -> (exp)
    Exp(Exp* e) : Node(e->name), type(e->type), reg(e->reg), truelist(e->truelist), falselist(e->falselist), nextlist(e->nextlist) {};

    //exp -> exp binop/relop/and/or exp
    Exp(Exp* e1, Node* op, Exp* e2, string markerLabel = "");

    //exp -> not exp
    Exp(Node* op, Exp* e);

    //exp -> num b
    Exp(Node* n, Node* b);

    //exp -> num/string/true/false
    Exp(Node* n, std::string type);

    //exp -> (type) exp
    Exp(Type* t, Exp* e);

    //exp -> id
    Exp(Node* id);

    //exp -> call
    Exp(Call* c) : Node(c->name), type(c->type), reg(c->reg), truelist(c->truelist), falselist(c->falselist), nextlist(c->nextlist) {}

    ~Exp() = default;
};

/*this was added to check the type of exp before going in statement.
checking the type in statement led to a wrong yyline printing.*/
void check_exp(Exp* exp);


class ExpList : public Node {
public:
    std::vector<Exp> expressions;

    //explist -> exp
    ExpList(Exp* e);

    //explist -> exp , explist
    ExpList(Exp* e, ExpList* list);

    ~ExpList() = default;
};


class Statement : public Node {
public:
    bplist continue_list = {};
    bplist break_list = {};
    bplist next_list = {};

    //statement -> { statements }
    Statement(Statements* sts);

    //statement -> type ID ;
    Statement(Type* t, Node* id);

    //statement -> type ID = Exp ;
    Statement(Type* t, Node* id, Exp* e);

    /*statement -> ID = Exp ;
    /statement -> return exp;*/
    Statement(Node* id, Exp* e);

    //statement -> call ;
    Statement(Call* c) : Node(c->type){}

    //statement -> return ;
    //statement -> break ;
    //statement -> continue ;
    Statement(Node* n);

    //statement -> if ( exp ) statement
    Statement(Exp* e, MarkerM* body, Statement* st);

    //statement -> if ( exp ) statement else statement
    Statement(Exp* e, MarkerM* if_body_marker, Statement* if_st, MarkerM* else_body_marker, Statement* else_st);

    //statement -> while (exp) { statement } ;
    Statement(Exp* e, MarkerM* cond_marker, Statement* body, MarkerM* body_marker);

    ~Statement() = default;

};

class Statements : public Node {
public:
    bplist continue_list = {};
    bplist break_list = {};
    bplist next_list = {};

    //Statements -> Statement
    Statements(Statement* st);

    //Statements -> Statements Statement
    Statements(Statements* sts, Statement* st);

    ~Statements() = default;

};

class Program : public Node{
public:

    //Program -> Funcs
    Program() {}

    ~Program() = default;
};

class Funcs : public Node{
public:

    //Funcs -> FuncDecl Funcs | epsilon
    Funcs() {}
    ~Funcs() = default;
};

class OverRide: public Node{
public:
    bool isOverRide;

    //OverRide -> OVERRIDE | epsilon
    OverRide(bool answer = false): isOverRide(answer) {}
    ~OverRide() = default;
};

class RetType : public Node{
public:
    std::string type;

    //RetType -> Type | VOID 
    RetType(std::string t) : type(t) {}
    ~RetType() = default;
};

class FormalDecl : public Node{
public:
    std::string type;

    //FuncDecl -> OverRide RetType ID ( Formals )  { Statements }
    FormalDecl(Type* t, Node* node) : type(t->type), Node(node->name) {}
    ~FormalDecl() = default;
};

class FormalsList : public Node{
public:
    std::vector<FormalDecl> param_list;

    //FormalsList -> FormalDecl , FormalsList
    FormalsList(FormalDecl* dec, FormalsList* list);

    //FormalsList -> FormalDecl
    FormalsList(FormalDecl* dec);
    ~FormalsList() = default;
};

class Formals: public Node{
public:
    std::vector<FormalDecl> param_list;

    //Formals -> FormalsList
    Formals(FormalsList* fl, int l_n) : param_list(fl->param_list) {}

    //Formals -> epsilon
    Formals() {}
    ~Formals() = default;

};

class FuncDecl : public Node {
public:

    ////FormalDecl -> Type ID
    FuncDecl(OverRide* override, RetType* rt, Node* id, Formals* params);
    ~FuncDecl() = default;
};

class MarkerM : public Node {
public:
    MarkerM();
    ~MarkerM() = default;
};


#endif //HW3_OUTPUT_CPP_PARSER_H
