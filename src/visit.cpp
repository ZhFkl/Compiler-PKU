#include "visit.h"
#include <iostream>
#include <string>
#include <cassert>
using namespace std;

void Visit(const koopa_raw_program_t &program){
    Visit(program.values);
    Visit(program.funcs);

}

void Visit(const koopa_raw_slice_t &slice){
    for(size_t i = 0; i < slice.len; i++){
        auto ptr = slice.buffer[i];
        switch(slice.kind){
            case KOOPA_RSIK_FUNCTION:
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                assert(false && "未知的slice类型");
        }
    }
}

void Visit(const koopa_raw_function_t & func){
    if(func->bbs.len == 0) return;
    string name = func->name + 1;
    cout << "\t.text" << endl;
    cout << "\t.globl " << name << endl;
    cout << name << ":" << endl;
    for(size_t i = 0; i < func->bbs.len; i++){
        assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        Visit(bb);
    }
}



void Visit(const koopa_raw_basic_block_t &bb){
    for(size_t i = 0; i < bb->insts.len ;i++){
        assert(bb->insts.kind == KOOPA_RSIK_VALUE);
        koopa_raw_value_t insts = (koopa_raw_value_t) bb->insts.buffer[i];
        Visit(insts);
    }
}



void Visit(const koopa_raw_value_t &val){
    const auto& kind = val->kind;
    //根据指令value来进行操作
    //这里只是简单打印一下指令的tag，实际使用中可以根据tag来区分不同类型的指令进行处理
    switch(kind.tag){
        case KOOPA_RVT_INTEGER:
            Visit(kind.data.integer);
            break;
        case KOOPA_RVT_RETURN:
            Visit(kind.data.ret);
            break;
        default:
            assert(false);
    }
}



void Visit(const koopa_raw_integer_t &integer){
     cout << "\tli a0, " << integer.value << endl;
}



void Visit(const koopa_raw_return_t &ret){
    if(ret.value != nullptr){
        if(ret.value->kind.tag == KOOPA_RVT_INTEGER){
            Visit(ret.value->kind.data.integer);
        }    
        else {
            Visit(ret.value);
        }   
    }
    cout << "\tret" << endl;
}