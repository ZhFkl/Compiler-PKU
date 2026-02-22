
#pragma once
//这个文件主要是用来定义此时的endf的结构
/*CompUnit  ::= FuncDef;

FuncDef   ::= FuncType IDENT "(" ")" Block;
FuncType  ::= "int";

Block     ::= "{" Stmt "}";
Stmt      ::= "return" Number ";";
Number    ::= INT_CONST;
*/
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

inline int koopa_tmp_cnt = 0;
inline string koopa_insts_buffer = "";


class BaseAST {
//这个是基类，要提供之后的接口也可以是一个纯虚函数
//我这里要提供一个什么接口？
    public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual string GenKoopaIR() const = 0;
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
    unique_ptr<BaseAST> stmt;
    void Dump() const override {
        cout << "BlockAST: {";
        stmt->Dump();
        cout << "}";
    }

    string GenKoopaIR() const override {
        cout << "BlockAST GenKoopaIR" << endl;
        return stmt->GenKoopaIR();
    }
};

class StmtAST : public BaseAST {
    public:
    unique_ptr<BaseAST> exp;
    void Dump() const override {
        cout << "StmtAST: return ";
        exp->Dump();
        cout << ";";
    } 

    string GenKoopaIR() const override {
        cout << "StmtAST GenKoopaIR" << endl;
        string val_name = exp->GenKoopaIR();
        koopa_insts_buffer += "  ret " + val_name + "\n";
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
};

class PrimaryExpAST : public BaseAST {
    public:
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> number;
        void Dump() const override {
            cout << "PrimaryExpAST: ";
            if(exp){
                exp->Dump();
            } else {
                number->Dump();
            }
        }

        string GenKoopaIR() const override {
            cout << "PrimaryExpAST GenKoopaIR" << endl;
            if(exp){
                return exp->GenKoopaIR();
            } else {
                return number->GenKoopaIR();
            }
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
};

//定义functype
//定义block
//定义stmt
//定义number
