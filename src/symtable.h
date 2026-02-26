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
