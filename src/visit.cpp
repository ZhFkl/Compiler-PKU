#include "visit.h"
#include <iostream>
#include <string>
#include <cassert>
#include <unordered_map>
using namespace std;

unordered_map<koopa_raw_value_t,int> stack_map;
int current_stack_frame_size = 0;

void load_value(koopa_raw_value_t val,const string&reg){
    if(val->kind.tag == KOOPA_RVT_INTEGER){
       cout << "\tli " << reg << ", " << val->kind.data.integer.value << endl;
    }else {
        //此时是从栈上来的值
        assert(stack_map.find(val) != stack_map.end() && "访问了未分配的值");
        int offset = stack_map[val];
        cout << "\tlw " << reg << ", " << offset << "(sp)" << endl;
    }
}



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


    //栈分配空间
    stack_map.clear();
    int current_stack_offset = 0;//此时的current_stack offset 是为了之后的栈对齐

    for(size_t i = 0; i < func->bbs.len; i++){
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        for(size_t j = 0; j < bb->insts.len ;j++){
            koopa_raw_value_t insts = (koopa_raw_value_t) bb->insts.buffer[j];
            if(insts->ty->tag != KOOPA_RTT_UNIT){
                stack_map[insts] = current_stack_offset;
                current_stack_offset += 4;
            }
        }
    }
    //计算对其的栈指针
    current_stack_frame_size = ((current_stack_offset + 15) / 16) * 16;
    if(current_stack_frame_size > 0){
        //分配栈空间
        cout << "\taddi sp, sp, -" << current_stack_frame_size << endl;
    }



    //栈空间分配完毕开始执行block解析
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
    //cout << kind.tag << endl;
    //根据指令value来进行操作
    //这里只是简单打印一下指令的tag，实际使用中可以根据tag来区分不同类型的指令进行处理
    switch(kind.tag){
        case KOOPA_RVT_INTEGER:
            //Visit(kind.data.integer);
            break;
        case KOOPA_RVT_RETURN:
            Visit(kind.data.ret);
            break;
        case  KOOPA_RVT_BINARY:
            Visit(val, kind.data.binary);
            break;
        case KOOPA_RVT_LOAD:
            Visit(val, kind.data.load);
            break;
        case KOOPA_RVT_STORE:
            Visit(val, kind.data.store);
            break;

        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            break;
        default:
            assert(false);
    }
}


/*
void Visit(const koopa_raw_integer_t &integer){
     cout << "\tli a0, " << integer.value << endl;
}
*/



void Visit(const koopa_raw_return_t &ret){
    // 返回值需要被放入 a0 寄存器
    if(ret.value != nullptr){
        load_value(ret.value, "a0");
    }
    
    // 恢复栈指针 (与函数开头的分配对称)
    if (current_stack_frame_size > 0) {
        cout << "\taddi sp, sp, " << current_stack_frame_size << endl;
    }
    
    // 生成 ret 指令
    cout << "\tret" << endl;
}

void Visit(const koopa_raw_value_t &val, const koopa_raw_binary_t &binary){
    //先加载到我的寄存器中
    load_value(binary.lhs, "t0");
    load_value(binary.rhs, "t1");

    //根据不同的操作符来生成不同的指令
    switch(binary.op){
  // 算术运算
        case KOOPA_RBO_ADD: cout << "\tadd t0, t0, t1" << endl; break;
        case KOOPA_RBO_SUB: cout << "\tsub t0, t0, t1" << endl; break;
        case KOOPA_RBO_MUL: cout << "\tmul t0, t0, t1" << endl; break;
        case KOOPA_RBO_DIV: cout << "\tdiv t0, t0, t1" << endl; break;
        case KOOPA_RBO_MOD: cout << "\trem t0, t0, t1" << endl; break;
        
        // 逻辑/位运算
        case KOOPA_RBO_AND: cout << "\tand t0, t0, t1" << endl; break;
        case KOOPA_RBO_OR:  cout << "\tor t0, t0, t1" << endl; break;
        case KOOPA_RBO_XOR: cout << "\txor t0, t0, t1" << endl; break;
        case KOOPA_RBO_SHL: cout << "\tsll t0, t0, t1" << endl; break;
        case KOOPA_RBO_SHR: cout << "\tsrl t0, t0, t1" << endl; break;
        case KOOPA_RBO_SAR: cout << "\tsra t0, t0, t1" << endl; break;

        // 比较运算 (RISC-V 没有直接的 <=, >= 等，需要组合指令)
        case KOOPA_RBO_EQ: 
            cout << "\txor t0, t0, t1" << endl;
            cout << "\tseqz t0, t0" << endl; 
            break;
        case KOOPA_RBO_NOT_EQ: 
            cout << "\txor t0, t0, t1" << endl;
            cout << "\tsnez t0, t0" << endl; 
            break;
        case KOOPA_RBO_LT: cout << "\tslt t0, t0, t1" << endl; break;
        case KOOPA_RBO_GT: cout << "\tsgt t0, t0, t1" << endl; break;
        case KOOPA_RBO_LE: // <= 等价于 !(> )
            cout << "\tsgt t0, t0, t1" << endl;
            cout << "\txori t0, t0, 1" << endl; 
            break;
        case KOOPA_RBO_GE: // >= 等价于 !(< )
            cout << "\tslt t0, t0, t1" << endl;
            cout << "\txori t0, t0, 1" << endl; 
            break;
        default:
            assert(false && "未实现的二元操作");
    }
    //把t0中的结果保存到对应的栈中
    int offset = stack_map[val];
    cout << "\tsw t0, " << offset << "(sp)" << endl;
}


void Visit(const koopa_raw_value_t &val, const koopa_raw_load_t &load){
    int offset = stack_map[load.src];
    cout << "\tlw t0, " << offset << "(sp)" << endl;
    int val_offset = stack_map[val];
    cout << "\tsw t0, " << val_offset << "(sp)" << endl;
}

void Visit(const koopa_raw_value_t &val, const koopa_raw_store_t &store){
    load_value(store.value, "t0");
    int offset = stack_map[store.dest];
    cout << "\tsw t0, " << offset << "(sp)" << endl;
}