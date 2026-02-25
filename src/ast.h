
#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
using namespace std;

enum class SymbolType{
    VARIABLE,
    CONSTANT
};

struct SymbolEntry
{
    SymbolType type;
    int int_val;
    string var_name;
    /* data */
};



inline int koopa_tmp_cnt = 0;
inline string koopa_insts_buffer = "";
inline unordered_map<string,SymbolEntry> symbol_table;



class BaseAST {
//这个是基类，要提供之后的接口也可以是一个纯虚函数
//我这里要提供一个什么接口？
    public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual string GenKoopaIR() const = 0;
    virtual int CalcValue() const {
        cerr << "CalcValue not implemented for this AST node!" << endl;
        return 0;
    }
};

//定义此时的compUnit、
class CompUnitAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        cout << "CompUnitAST: {" ;
        func_def->Dump();
        cout << "}" << endl;
    }

    string GenKoopaIR() const override{
        return func_def->GenKoopaIR();
    }
    
};

class FuncDefAST :public BaseAST {
    public:
    std:: unique_ptr<BaseAST> func_type;
    std:: string ident;
    std:: unique_ptr<BaseAST> block;

    void Dump() const override {
        cout << "FuncDefASt :";
        func_type->Dump();
        cout << "," << ident << ",";
        block->Dump();
        cout << "}";
    }

    string GenKoopaIR() const override {
        ostringstream ss;
        ss << "fun @" << ident << "(): i32 {" << endl;
        ss << "%entry:" << endl;
        block->GenKoopaIR();
        ss << koopa_insts_buffer <<endl;
        ss << "}" << endl;
        cout << ss.str();
        return ss.str();
        //
    }
};

class FuncTypeAST : public BaseAST {
    public:
    string type = "int";

    void Dump() const override {
        cout << "FuncTypeAST: " << type;
    }

    string GenKoopaIR() const override {
        return type;
    }
};

class BlockAST : public BaseAST {
    public:
    vector<unique_ptr<BaseAST>> block_items;
    void Dump() const override {
        cout << "BlockAST: {";
        for(auto &item: block_items){
            item->Dump();
            cout << ", ";
        }
        cout << "}";
    }

    string GenKoopaIR() const override {
        for(const auto &item: block_items){
            item->GenKoopaIR();
        }
        return "";
    }
};

class BlockItemAST : public BaseAST {
    public:
    unique_ptr<BaseAST> decl;
    unique_ptr<BaseAST> stmt;
    void Dump() const override {
        cout << "BlockItemAST: ";
        if(decl){
            decl->Dump();
        } else if(stmt){
            stmt->Dump();
        }
    }

    string GenKoopaIR() const override {
        cout << "BlockItemAST GenKoopaIR" << endl;
        if(decl){
            return decl->GenKoopaIR();
        } else if(stmt){
        return stmt->GenKoopaIR();
        }
        return "";
    }
};

class StmtAST : public BaseAST {
    public:
    bool is_return = false;
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;
    void Dump() const override {
        if(is_return){
            cout << "StmtAST: return ";
            exp->Dump();
        } else {
            cout << "StmtAST: ";
            lval->Dump();
        }
    } 

    string GenKoopaIR() const override {
        if(is_return){
            string ret_val = exp->GenKoopaIR();
            koopa_insts_buffer += "  ret " + ret_val + "\n";
            return "";
        } else {
            string lval_name = lval->GenKoopaIR();
            auto it = symbol_table.find(lval_name);
            if (it == symbol_table.end()) {
                cerr << "Semantic Error: Undefined variable '" << lval_name << "'" << endl;
                exit(1);
            }
            if (it->second.type == SymbolType::CONSTANT) {
                cerr << "Semantic Error: Cannot assign to constant '" << lval_name << "'" << endl;
                exit(1);
            }

            // 生成求值指令，并把值存入该变量的内存中
            string val_name = exp->GenKoopaIR();
            koopa_insts_buffer += "  store " + val_name + ", " + it->second.var_name + "\n";

        }
        return "";
        
    }
};

class NumberAST :public BaseAST {
    public:
    int value;
    void Dump() const override {
        cout << "NumberAST: " << value;
    }

    string GenKoopaIR() const override {
        cout << "NumberAST GenKoopaIR" << endl;
        return to_string(value);
    }

    int CalcValue() const override {
        return value;
    }
};

class ExpAST : public BaseAST {
    public:
        unique_ptr<BaseAST> lor_exp;
        void Dump() const override {
            cout << "ExpAST: ";
            lor_exp->Dump();
        }
        string GenKoopaIR() const override {
            cout << "ExpAST GenKoopaIR" << endl;
            return lor_exp->GenKoopaIR();
        }

        int CalcValue() const override {
            return lor_exp->CalcValue();
        }
};

class PrimaryExpAST : public BaseAST {
    public:
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> number;
        unique_ptr<BaseAST> LVal;
        void Dump() const override {
            cout << "PrimaryExpAST: ";
            if(exp){
                exp->Dump();
            } else if(number){
                number->Dump();
            } else if(LVal){
                LVal->Dump();
            }
        }

        string GenKoopaIR() const override {
            cout << "PrimaryExpAST GenKoopaIR" << endl;
            if(exp){
                return exp->GenKoopaIR();
            } else if(number){
                return number->GenKoopaIR();
            } else if(LVal){
                return LVal->GenKoopaIR();
            }
            return "";
        }

        int CalcValue() const override {
            if(exp){
                return exp->CalcValue();
            } else if(number){
                return number->CalcValue();
            } else if(LVal){
                return LVal->CalcValue();
            }
            return 0;
        }
};


class LValAST : public BaseAST {
    public:
        string ident;
        void Dump() const override {
            cout << "LValAST: " << ident;
        }

        string GenKoopaIR() const override {
            auto it = symbol_table.find(ident);
            if (it == symbol_table.end()) {
                cerr << "Semantic Error: Undefined symbol '" << ident << "'" << endl;
                exit(1);
            }

            if (it->second.type == SymbolType::CONSTANT) {
                // 常量直接返回数值字符串
                return to_string(it->second.int_val);
            } else {
                // 变量！必须生成 load 指令从内存取值
                string tmp_var = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + tmp_var + " = load " + it->second.var_name + "\n";
                return tmp_var; // 返回装载了值的新临时寄存器名
            }
        }

        int CalcValue() const override {
            auto it = symbol_table.find(ident);
            if (it == symbol_table.end()) {
                cerr << "Semantic Error: Undefined symbol '" << ident << "'" << endl;
                exit(1);
            }
            // 语义分析：编译期常量求值时遇到了变量，报错！
            if (it->second.type == SymbolType::VARIABLE) {
                cerr << "Semantic Error: Variable '" << ident << "' cannot be used in constant expression" << endl;
                exit(1);
            }
            return it->second.int_val;
        }
};


class UnaryExpAST : public BaseAST {
    public:
        unique_ptr<BaseAST> primary_exp;
        char op = 0;
        unique_ptr<BaseAST> unary_exp;
        void Dump() const override {
            cout << "UnaryExpAST: ";
            if(primary_exp){
                primary_exp->Dump();
            } else if(op && unary_exp){
                cout << op << " ";
                unary_exp->Dump();
            }
        }

        string GenKoopaIR() const override {
            cout << "UnaryExpAST GenKoopaIR" << endl;
            if(primary_exp){
                return primary_exp->GenKoopaIR();
            } else if(op && unary_exp){
                string inner_val = unary_exp->GenKoopaIR();
                if(op == '+'){
                    return inner_val;
                }
                string res_var = "%" + to_string(koopa_tmp_cnt++);
                if(op == '-'){
                    koopa_insts_buffer += "  " + res_var + " = sub 0, " + inner_val + "\n";
                }else if(op == '!'){
                    koopa_insts_buffer += "  " + res_var + " = eq " + inner_val + ", 0\n";
                }
                return res_var;
            }
            return "";
        }

        int CalcValue() const override {
            if(primary_exp) return primary_exp->CalcValue();
            int val = unary_exp->CalcValue();
            if(op == '+') return val;
            if(op == '-') return -val;
            if(op == '!') return !val;
            return 0;
        }
};

class AddExpAST : public BaseAST {
    public:
    unique_ptr<BaseAST> mul_exp;
    unique_ptr<BaseAST> add_exp;
    char op = 0; // '+' or '-'
    void Dump() const override {
        cout << "AddExpAST: ";
        if(add_exp){
            add_exp->Dump();
            cout << " " << op << " ";
            mul_exp->Dump();
        }else{
            mul_exp->Dump();
        }
    }

    string GenKoopaIR() const override {
        if(add_exp){
            string left_val = add_exp->GenKoopaIR();
            string right_val = mul_exp->GenKoopaIR();
            string res_var = "%" + to_string(koopa_tmp_cnt++);
            if(op == '+'){
                koopa_insts_buffer += "  " + res_var + " = add " + left_val + ", " + right_val + "\n";
            } else if(op == '-'){
                koopa_insts_buffer += "  " + res_var + " = sub " + left_val + ", " + right_val + "\n";
            }
            return res_var;
        } else {
            return mul_exp->GenKoopaIR();
        }
    }

    int CalcValue() const override {
        if(!add_exp) return mul_exp->CalcValue();
        int left = add_exp->CalcValue();
        int right = mul_exp->CalcValue();
        if(op == '+') return left + right;
        if(op == '-') return left - right;
        return 0;
    }
};

class MulExpAST : public BaseAST {
    public:
    unique_ptr<BaseAST> unary_exp;
    unique_ptr<BaseAST> mul_exp;
    char op = 0;
    void Dump() const override {
        cout << "MulExpAST: ";
        if(mul_exp){
            mul_exp->Dump();
            cout << " " << op << " ";
            unary_exp->Dump();
        }else {
            unary_exp->Dump();
        }
    }
    
    string GenKoopaIR() const override {
        if(mul_exp){
            string left_val = mul_exp->GenKoopaIR();
            string right_val = unary_exp->GenKoopaIR();
            string res_var = "%" + to_string(koopa_tmp_cnt++);
            if(op == '*'){
                koopa_insts_buffer += "  " + res_var + " = mul " + left_val + ", " + right_val + "\n";
            } else if(op == '/'){
                koopa_insts_buffer += "  " + res_var + " = div " + left_val + ", " + right_val + "\n";
            } else if(op == '%'){
                koopa_insts_buffer += "  " + res_var + " = mod " + left_val + ", " + right_val + "\n";
            }
            return res_var;
        }else {
            return unary_exp->GenKoopaIR();
        }
    }

     int CalcValue() const override {
        if(!mul_exp) return unary_exp->CalcValue();
        int left = mul_exp->CalcValue();
        int right = unary_exp->CalcValue();
        if(op == '*') return left * right;
        if(op == '/') return left / right;
        if(op == '%') return left % right;
        return 0;
    }
};

class RelExp : public BaseAST{
    public:
    unique_ptr<BaseAST> add_exp;
    unique_ptr<BaseAST> rel_exp;
    string op = ""; // '<', '>', '<=', '>='
    void Dump() const override {
        cout << "RelExpAST: ";
        if(rel_exp){
            rel_exp->Dump();
            cout << " " << op << " ";
            add_exp->Dump();
        } else {
            add_exp->Dump();
        }
    }

    string GenKoopaIR() const override {
        if(rel_exp){
            string left_val = rel_exp->GenKoopaIR();
            string right_val = add_exp->GenKoopaIR();
            string res_var = "%" + to_string(koopa_tmp_cnt++);
            if(op == "<"){
                koopa_insts_buffer += "  " + res_var + " = lt " + left_val + ", " + right_val + "\n";
            } else if(op == ">"){
                koopa_insts_buffer += "  " + res_var + " = gt " + left_val + ", " + right_val + "\n";
            } else if(op == "<="){
                koopa_insts_buffer += "  " + res_var + " = le " + left_val + ", " + right_val + "\n";
            } else if(op == ">="){
                koopa_insts_buffer += "  " + res_var + " = ge " + left_val + ", " + right_val + "\n";
            }
            return res_var;
        } else {
            return add_exp->GenKoopaIR();
        }
    }

    int CalcValue() const override {
        if(!rel_exp) return add_exp->CalcValue();
        int left = rel_exp->CalcValue();
        int right = add_exp->CalcValue();
        if(op == "<") return left < right;
        else if(op == ">") return left > right;
        else if(op == "<=") return left <= right;
        else if(op == ">=") return left >= right;
        return 0;
    }
};

class EqExp : public BaseAST{
    public:
        unique_ptr<BaseAST> rel_exp;
        unique_ptr<BaseAST> eq_exp;
        string op = ""; // '==' or '!='
        void Dump() const override {
            cout << "EqExpAST: ";
            if(eq_exp){
                eq_exp->Dump();
                cout << " " << op << " ";
                rel_exp->Dump();
            } else {
                rel_exp->Dump();
            }
        }

        string GenKoopaIR() const override {
            if(eq_exp){
                string left_val = eq_exp->GenKoopaIR();
                string right_val = rel_exp->GenKoopaIR();
                string res_var = "%" + to_string(koopa_tmp_cnt++);
                if(op == "=="){
                    koopa_insts_buffer += "  " + res_var + " = eq " + left_val + ", " + right_val + "\n";
                } else if(op == "!="){
                    koopa_insts_buffer += "  " + res_var + " = ne " + left_val + ", " + right_val + "\n";
                }
                return res_var;
            } else {
                return rel_exp->GenKoopaIR();
            }
        }

        int CalcValue() const override {
            if(!eq_exp) return rel_exp->CalcValue();
            int left = eq_exp->CalcValue();
            int right = rel_exp->CalcValue();
            return (op == "==") ? (left == right) : (left != right);
        }
};

class LAndExp : public BaseAST {
    public:
        unique_ptr<BaseAST> eq_exp;
        unique_ptr<BaseAST> land_exp;
        void Dump() const override {
            cout << "LAndExpAST: ";
            if(land_exp){
                land_exp->Dump();
                cout << " && ";
                eq_exp->Dump();
            } else {
                eq_exp->Dump();
            }
        }

        string GenKoopaIR() const override {
            if(land_exp){
                string left_val = land_exp->GenKoopaIR();
                string right_val = eq_exp->GenKoopaIR();
                string left_bool = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + left_bool + " = ne " + left_val + ", 0\n";
        

                string right_bool = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + right_bool + " = ne " + right_val + ", 0\n";
                
                string res_var = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + res_var + " = and " + left_bool + ", " + right_bool + "\n";
                return res_var;
            } else {
                return eq_exp->GenKoopaIR();
            }
        }

        int CalcValue() const override {
            if(land_exp){
                return (land_exp->CalcValue() != 0) && (eq_exp->CalcValue() != 0);
            } else {
                return eq_exp->CalcValue();
            }
        }
};

class LOrExp : public BaseAST {
    public:
        unique_ptr<BaseAST> land_exp;
        unique_ptr<BaseAST> lor_exp;
        void Dump() const override {
            cout << "LOrExpAST: ";
            if(lor_exp){
                lor_exp->Dump();
                cout << " || ";
                land_exp->Dump();
            } else {
                land_exp->Dump();
            }
        }

        string GenKoopaIR() const override {
            if(lor_exp){
                string left_val = land_exp->GenKoopaIR();
                string right_val = lor_exp->GenKoopaIR();
                string left_bool = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + left_bool + " = ne " + left_val + ", 0\n";
        

                string right_bool = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + right_bool + " = ne " + right_val + ", 0\n";
                
                string res_var = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + res_var + " = or " + left_bool + ", " + right_bool + "\n";
                return res_var;
            } else {
                return land_exp->GenKoopaIR();
            }
        }

        int CalcValue() const override {
            if(lor_exp){
                return (land_exp->CalcValue() != 0) || (lor_exp->CalcValue() != 0);
            } else {
                return land_exp->CalcValue();
            }
        }
};

class DeclAST : public BaseAST {
    public:
        unique_ptr<BaseAST> const_decl;
        unique_ptr<BaseAST> var_decl;
        void Dump() const override {
            cout << "DeclAST: ";
            if(const_decl){
                const_decl->Dump();
            } else if(var_decl){
                var_decl->Dump();
            }   
        }
        string GenKoopaIR() const override {
            if(const_decl) {
                return const_decl->GenKoopaIR();
            } else {
                return var_decl->GenKoopaIR();
            }
        }
};

class ConstDeclAST : public BaseAST {
    public:
        std::vector<std::unique_ptr<BaseAST>> const_defs;
        unique_ptr<BaseAST> b_type;
        void Dump() const override {
            cout << "ConstDeclAST: ";
            b_type->Dump();
            for (const auto& def : const_defs) {
                def->Dump();
                cout << ", ";
            }
        }
        string GenKoopaIR() const override {
            for (const auto& def : const_defs) {
                def->GenKoopaIR();
            }
            return "";
        }
};

class BTypeAST : public BaseAST {
    public:
        string type = "int";
        void Dump() const override {
            cout << "BTypeAST: " << type;
        }
        string GenKoopaIR() const override {
            return type;
        }
};

class ConstDefAST : public BaseAST {    
    public:
        string ident;
        unique_ptr<BaseAST> const_init_val;
        void Dump() const override {
            cout << "ConstDefAST: " << ident << " = ";
            const_init_val->Dump();
        }

        string GenKoopaIR() const override {
            if (symbol_table.find(ident) != symbol_table.end()) {
                cerr << "Semantic Error: Redefinition of symbol '" << ident << "'" << endl;
                exit(1);
            }

            int real_value = const_init_val->CalcValue();
            // 存入符号表，标记为 CONSTANT
            symbol_table[ident] = {SymbolType::CONSTANT, real_value, ""};
            return "";  
        }
};

class ConstInitValAST : public BaseAST {
    public:
        unique_ptr<BaseAST> const_exp;
        void Dump() const override {
            cout << "ConstInitValAST: ";
            const_exp->Dump();
        }
        string GenKoopaIR() const override {
            return const_exp->GenKoopaIR();
        }

        int CalcValue() const override {
            return const_exp->CalcValue();
        }
};

class ConstExpAST : public BaseAST {
    public:
        unique_ptr<BaseAST> exp;
        void Dump() const override {
            cout << "ConstExpAST: ";
            exp->Dump();
        }
        string GenKoopaIR() const override {
            return exp->GenKoopaIR();
        }

        int CalcValue() const override {
            return exp->CalcValue();
        }
};

class VarDeclAST : public BaseAST {
    public:
        vector<unique_ptr<BaseAST> > var_defs;
        unique_ptr<BaseAST> b_type;
        void Dump() const override {
            cout << "VarDeclAST: ";
            b_type->Dump();
            for(const auto& def: var_defs){
                def->Dump();
                cout << ", ";
            }
        }

        string GenKoopaIR() const override {
            for(const auto& def: var_defs){
                def->GenKoopaIR();
            }
            return "";
        }
};

class VarDefAST : public BaseAST {
    public:
        string ident;
        unique_ptr<BaseAST> init_val; // 可以为 nullptr，表示未初始化
        void Dump() const override {
            cout << "VarDefAST: " << ident;
            if(init_val) { cout << " = "; init_val->Dump(); }
        }

        string GenKoopaIR() const override {
            //检查此时的语义是否重复
            if(symbol_table.find(ident) != symbol_table.end()){
                cerr << "Error: Variable " << ident << " already defined!" << endl;
                return "";
            }
            //为变量生成一个koopa IR中的临时变量名
            string var_name = "%" + ident + "_" + to_string(koopa_tmp_cnt++);
            symbol_table[ident] = {SymbolType::VARIABLE, 0, var_name};
            koopa_insts_buffer += "  " + var_name + " = alloc i32\n";
            if(init_val){
                string init_val_name = init_val->GenKoopaIR();
                koopa_insts_buffer += "  store " + init_val_name + ", " + var_name + "\n";
            }
            return "";
        }
};

class InitValAST : public BaseAST {
    public:
        unique_ptr<BaseAST> exp;
        void Dump() const override {
            cout << "InitValAST: ";
            exp->Dump();
        }

        string GenKoopaIR() const override {
            return exp->GenKoopaIR();
        }

        int CalcValue() const override {
            return exp->CalcValue();
        }
};


//定义functype
//定义block
//定义stmt
//定义number
