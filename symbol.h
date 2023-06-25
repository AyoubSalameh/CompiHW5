#ifndef HW3_OUTPUT_CPP_SYMBOL_H
#define HW3_OUTPUT_CPP_SYMBOL_H
#include <string>
#include <stack>
#include <vector>

#define DEBUG false


using namespace std;



bool type_compatible(const string& right, const string& left);

///********************* SYMBOL TABLE ENTRY ********************************///
class symbol_table_entry {
public:
    string name;
    string uniqe_name; //if this is func, and is override, we will attach a digit to the functions name in oreder to deffiritiate. is there is no override it will be identical to name
    string type;  //if func, this means the return type
    int offset;

    bool is_func = false;
    bool is_override = false;
    vector<string> params;

public:
    symbol_table_entry(const string& n, const string& u_n, string t, int o, bool func = false, bool override = false,
                       vector<string> p = vector<string>()) :
                       name(n), uniqe_name(u_n), type(t), offset(o), is_func(func), is_override(override), params(p)
    {}
    ~symbol_table_entry() = default;
};


///********************* SYMBOL TABLE SCOPE ********************************///
class symbol_table_scope {
public:
    vector<symbol_table_entry> entries;
    bool is_loop;
    string func_ret_type; //if scope belongs to a function, this will hols its return type. will be an empty string otherwise.

    symbol_table_scope(bool loop, string ret): is_loop(loop), func_ret_type(ret) {}
    ~symbol_table_scope() = default;

    bool exists_in_scope(const symbol_table_entry& entry);
    void insert_to_scope(symbol_table_entry& entry);
    void print_scope(); //end scope, and calls helper function

    
    symbol_table_entry* get_variable_in_scope(const string& name);
    
    /*unused functions*/
    //bool symbol_declared_in_scope(const symbol_table_entry& entry);
    //symbol_table_entry* getSymbol()
    //symbol_table_entry* get_symbol_entry(const string& symbol_name); ///this is mainly to check main
};

///********************* TABLE STACK ********************************///
class table_stack {
public:
    vector<symbol_table_scope> tables_stack;
    stack<int> offsets_stack;
    int override_counter = 0;

    table_stack();
    ~table_stack() = default;

    void open_scope(bool is_loop = false, string ret_type = ""); //open a new empty scope
    void close_scope();
    void final_check();
    void insert_symbol(const string& n, string t, bool func = false, bool override = false,
                       vector<string> p = vector<string>());
    void insert_func_args(vector<string> types, vector<string> names, string retType);
    bool symbol_exists(const symbol_table_entry& entry);    ///used only in insert

    symbol_table_entry* get_variable(const string& name);
    symbol_table_entry* get_function(const string& name, vector<string> params = {});
    bool checkLoop();
    string get_closest_func_return_type();

    /*unused functions*/
    //bool symbol_declared(const symbol_table_entry& entry);

};





#endif //HW3_OUTPUT_CPP_SYMBOL_H
