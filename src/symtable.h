#pragma once
#include <string> 
#include <vector>
#include <unordered_map>
#include <iostream>
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

class SymbolTable {
    private:
        vector<unordered_map<string, SymbolEntry>> scopes;
    public:
    SymbolTable(){
        scopes.emplace_back(); // 添加全局作用域
    }

    void EnterScope(){
        scopes.push_back({});
    }

    void ExitScope(){
        if (scopes.size() > 1) {
            scopes.pop_back();
        } else {
            cerr << "Error: Cannot exit global scope!" << endl;
        }
    }

    bool Insert(const string& name, const SymbolEntry& entry){
        if (scopes.back().count(name) > 0) {
            cerr << "Error: Redefinition of symbol '" << name << "' in the same scope!" << endl;
            return false;
        } else {
            scopes.back()[name] = entry;
        }
        return true;
    }

    SymbolEntry* Lookup(const string& name){
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr; // 未找到
    }
};


class KoopaIRBuilder{
private:
    int tmp_cnt = 0;
    string alloc_buffer = "";
    string inst_buffer = "";
    bool is_block_closed = false;
public:
    void Reset(){
        tmp_cnt = 0;
        alloc_buffer = "";
        inst_buffer = "";
        is_block_closed = false;
    }

    int GetUniqueId(){
        return tmp_cnt++;
    }

    string GetTmpVar(){
        return "%" + to_string(GetUniqueId());
    }

    void AddAlloc(const string& inst){
        alloc_buffer += "  " + inst + "\n";
    }

    void AddInst(const string& inst){
        if(is_block_closed){
            cerr << "Error: Cannot add instruction to a closed block!" << endl;
            return;
        }
        inst_buffer += "  " + inst + "\n";
    }

    //终结指令
    void EndWithJump(const string& target){
        if(is_block_closed){
            cerr << "Error: Block is already closed!" << endl;
            return;
        }
        inst_buffer += "  jump " + target + "\n";
        is_block_closed = true;
    }

    void EndWithBranch(const string& cond, const string& true_label, const string& false_label){
        if(is_block_closed){
            cerr << "Error: Block is already closed!" << endl;
            return;
        }
        inst_buffer += "  br " + cond + ", " + true_label + ", " + false_label + "\n";
        is_block_closed = true;
    }

    //终结指令return
    void EndWithRet(const string& lable){
        inst_buffer += "  ret " + lable + "\n";
        is_block_closed = true;
    }

    void StartNewBlock(const string& label){
        if(!is_block_closed){
            cerr << "Error: Previous block is not closed yet!" << endl;
            return;
        }
        inst_buffer += label + ":\n";
        is_block_closed = false;
    }

    string BuildFunction(const string& func_name){
        if(!is_block_closed){
            cerr << "Error: Cannot build function with an open block!" << endl;
            return "";
        }
        string final_ir = "fun @" + func_name + "(): i32 {\n";
        final_ir += "%entry:\n";
        final_ir += alloc_buffer; // alloc 统一置顶
        final_ir += inst_buffer;
        final_ir += "}\n";
        return final_ir;
    }

    bool IsBlockClosed() const {
        return is_block_closed;
    }
};