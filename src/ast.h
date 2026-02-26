
#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "symtable.h"
using namespace std;


inline int koopa_tmp_cnt = 0;
inline string koopa_insts_buffer = "";
inline SymbolTable sym_table;


class BaseAST {
//这个是基类，要提供之后的接口也可以是一个纯虚函数
//我这里要提供一个什么接口？
    public:
    virtual ~BaseAST() = default;
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

    string GenKoopaIR() const override{
        return func_def->GenKoopaIR();
    }
    
};

class FuncDefAST :public BaseAST {
    public:
    std:: unique_ptr<BaseAST> func_type;
    std:: string ident;
    std:: unique_ptr<BaseAST> block;


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

    string GenKoopaIR() const override {
        return type;
    }
};

class BlockAST : public BaseAST {
    public:
    vector<unique_ptr<BaseAST>> block_items;

    string GenKoopaIR() const override {
        sym_table.EnterScope();
        for(const auto &item: block_items){
            item->GenKoopaIR();
        }
        sym_table.ExitScope();
        return "";
    }
};

class BlockItemAST : public BaseAST {
    public:
    unique_ptr<BaseAST> decl;
    unique_ptr<BaseAST> stmt;

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


class LValAST : public BaseAST {
    public:
        string ident;

        string GenKoopaIR() const override {
            auto entry = sym_table.Lookup(ident);
            if (!entry) {
                cerr << "Semantic Error: Undefined symbol '" << ident << "'" << endl;
                exit(1);
            }

            if (entry->type == SymbolType::CONSTANT) {
                // 常量直接返回数值字符串
                return to_string(entry->int_val);
            } else {
                // 变量必须生成 load 指令从内存取值
                string tmp_var = "%" + to_string(koopa_tmp_cnt++);
                koopa_insts_buffer += "  " + tmp_var + " = load " + entry->var_name + "\n";
                return tmp_var; 
            }
        }

        int CalcValue() const override {
            auto entry = sym_table.Lookup(ident);
            if (!entry) {
                cerr << "Semantic Error: Undefined symbol '" << ident << "'" << endl;
                exit(1);
            }
            if (entry->type == SymbolType::VARIABLE) {
                cerr << "Semantic Error: Variable '" << ident << "' cannot be used in constant expression" << endl;
                exit(1);
            }
            return entry->int_val;
        }
};


class StmtAST : public BaseAST {
    public:
    bool is_return = false;
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> lval;
    unique_ptr<BaseAST> block;

    string GenKoopaIR() const override {
        if(is_return){
            if(exp){
                string ret_val = exp->GenKoopaIR();
                koopa_insts_buffer += "  ret " + ret_val + "\n";
            }else{
                koopa_insts_buffer += "  ret 0\n";
            }
            return "";
        } else if(lval && exp){
            LValAST* lval_ptr = static_cast<LValAST*>(lval.get());
            string target_ident = lval_ptr->ident;
            
            auto entry = sym_table.Lookup(target_ident);
            if (!entry) {
                cerr << "Semantic Error: Undefined variable '" << target_ident << "'" << endl;
                exit(1);
            }
            if (entry->type == SymbolType::CONSTANT) {
                cerr << "Semantic Error: Cannot assign to constant '" << target_ident << "'" << endl;
                exit(1);
            }
            string val_name = exp->GenKoopaIR();
            koopa_insts_buffer += "  store " + val_name + ", " + entry->var_name + "\n";

        }else if(block){
            block->GenKoopaIR();
        }else if(exp){
            exp->GenKoopaIR();
        }else {

        }
        return "";
    }
};

class NumberAST :public BaseAST {
    public:
    int value;
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


class UnaryExpAST : public BaseAST {
    public:
        unique_ptr<BaseAST> primary_exp;
        char op = 0;
        unique_ptr<BaseAST> unary_exp;

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
        string GenKoopaIR() const override {
            return type;
        }
};

class ConstDefAST : public BaseAST {    
    public:
        string ident;
        unique_ptr<BaseAST> const_init_val;

        string GenKoopaIR() const override {
            int real_value = const_init_val->CalcValue();
            // 存入当前层次的符号表
            SymbolEntry entry = {SymbolType::CONSTANT, real_value, ""};
            if (!sym_table.Insert(ident, entry)) {
                cerr << "Semantic Error: Redefinition of symbol '" << ident << "'" << endl;
                exit(1);
            }
            return "";  
        }
};

class ConstInitValAST : public BaseAST {
    public:
        unique_ptr<BaseAST> const_exp;

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

        string GenKoopaIR() const override {

            //为变量生成一个koopa IR中的临时变量名
            string var_name = "%" + ident + "_" + to_string(koopa_tmp_cnt++);
            SymbolEntry entry = {SymbolType::VARIABLE, 0, var_name};
            if (!sym_table.Insert(ident, entry)) {
                cerr << "Semantic Error: Redefinition of symbol '" << ident << "'" << endl;
                exit(1);
            }
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
