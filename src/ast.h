//
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
        unique_ptr<BaseAST> unary_exp;
        void Dump() const override {
            cout << "ExpAST: ";
            unary_exp->Dump();
        }
        string GenKoopaIR() const override {
            cout << "ExpAST GenKoopaIR" << endl;
            return unary_exp->GenKoopaIR();
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

//定义functype
//定义block
//定义stmt
//定义number
