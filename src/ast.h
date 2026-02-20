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
        ss << block->GenKoopaIR();
        ss << "}" << endl;
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
        return stmt->GenKoopaIR();
    }
};

class StmtAST : public BaseAST {
    public:
    unique_ptr<BaseAST> number;
    void Dump() const override {
        cout << "StmtAST: return ";
        number->Dump();
        cout << ";";
    } 

    string GenKoopaIR() const override {
        ostringstream ss;
        ss << "  ret " << number->GenKoopaIR() << endl;
        return ss.str();
    }
};

class NumberAST :public BaseAST {
    public:
    int value;
    void Dump() const override {
        cout << "NumberAST: " << value;
    }

    string GenKoopaIR() const override {
        return to_string(value);
    }
};
//定义functype
//定义block
//定义stmt
//定义number
