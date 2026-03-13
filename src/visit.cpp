// --- START OF FILE visit.cpp ---
#include "visit.h"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;

AsmGenerator::AsmGenerator() = default;

void AsmGenerator::Generate(const koopa_raw_program_t &program){
    Visit(program);
}

// 获取类型在内存中的实际大小 (字节)
int AsmGenerator::GetTypeSize(koopa_raw_type_t ty) {
    switch (ty->tag) {
        case KOOPA_RTT_INT32: return 4;
        case KOOPA_RTT_UNIT: return 0;
        case KOOPA_RTT_ARRAY: return ty->data.array.len * GetTypeSize(ty->data.array.base);
        case KOOPA_RTT_POINTER: return 4;
        case KOOPA_RTT_FUNCTION: return 0;
        default: return 0;
    }
}

// 自动检测大范围偏移量的 store 指令
void AsmGenerator::store_value(const string& reg, int offset) {
    if (offset >= -2048 && offset <= 2047) {
        cout << "\tsw " << reg << ", " << offset << "(sp)" << endl;
    } else {
        cout << "\tli t4, " << offset << endl;
        cout << "\tadd t4, sp, t4" << endl;
        cout << "\tsw " << reg << ", 0(t4)" << endl;
    }
}

// 高阶重构 load_value：可直接返回整数、局部内存指针、全局变量内存指针
void AsmGenerator::load_value(koopa_raw_value_t val, const string& reg, int sp_offset){
    if(val->kind.tag == KOOPA_RVT_INTEGER){
       cout << "\tli " << reg << ", " << val->kind.data.integer.value << endl;
    } else if (val->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        string name = val->name + 1;
        cout << "\tla " << reg << ", " << name << endl;
    } else if (val->kind.tag == KOOPA_RVT_ALLOC) {
        // 对于局部数组/变量分配，其实际值即为其栈上的起始地址
        int offset = stack_map[val] + sp_offset;
        if (offset >= -2048 && offset <= 2047) {
            cout << "\taddi " << reg << ", sp, " << offset << endl;
        } else {
            cout << "\tli " << reg << ", " << offset << endl;
            cout << "\tadd " << reg << ", sp, " << reg << endl;
        }
    } else {
        assert(stack_map.find(val) != stack_map.end() && "访问了未分配的值");
        int offset = stack_map[val] + sp_offset;
        if (offset >= -2048 && offset <= 2047) {
            cout << "\tlw " << reg << ", " << offset << "(sp)" << endl;
        } else {
            cout << "\tli t4, " << offset << endl;
            cout << "\tadd t4, sp, t4" << endl;
            cout << "\tlw " << reg << ", 0(t4)" << endl;
        }
    }
}

string AsmGenerator::GetBasicBlockLabel(koopa_raw_basic_block_t bb) {
    if (!bb || !bb->name) {
        static int anon_count = 0;
        return ".L_" + current_func_name + "_anon_" + std::to_string(anon_count++);
    }
    std::string name = bb->name;
    if (!name.empty() && name[0] == '%') {
        name = name.substr(1);  
    }
    return ".L_" + current_func_name + "_" + name;
}

bool AsmGenerator::HasCallINFunc(const koopa_raw_function_t &func){
    for(size_t i = 0; i < func->bbs.len; i++){
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        for(size_t j = 0; j < bb->insts.len ;j++){
            koopa_raw_value_t inst = (koopa_raw_value_t) bb->insts.buffer[j];
            if(inst->kind.tag == KOOPA_RVT_CALL){
                return true;
            }
        }
    }
    return false;
}

int AsmGenerator::AllocStackSpace(int size){
    int offset = current_stack_offset;
    current_stack_offset += size;
    return offset;
}

void AsmGenerator::Visit(const koopa_raw_program_t &program){
    Visit(program.values);
    Visit(program.funcs);
}

void AsmGenerator::Visit(const koopa_raw_slice_t &slice){
    for(size_t i = 0; i < slice.len; i++){
        auto ptr = slice.buffer[i];
        switch(slice.kind){
            case KOOPA_RSIK_FUNCTION: Visit(reinterpret_cast<koopa_raw_function_t>(ptr)); break;
            case KOOPA_RSIK_BASIC_BLOCK: Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr)); break;
            case KOOPA_RSIK_VALUE: Visit(reinterpret_cast<koopa_raw_value_t>(ptr)); break;
            default: assert(false && "未知的slice类型");
        }
    }
}

void AsmGenerator::Visit(const koopa_raw_function_t & func){
    if(func->bbs.len == 0) return;
    string name = func->name + 1;
    current_func_name = name;
    cout << "\t.text" << endl;
    cout << "\t.globl " << name << endl;
    cout << name << ":" << endl;

    stack_map.clear();
    current_stack_offset = 0;
    use_fp = false;
    fp_offset = 0;

    cur_func_need_save_ra = HasCallINFunc(func);
    cur_func_ra_offset = -1;

    bool need_fp = (func->params.len > 8); 

    if (cur_func_need_save_ra) {
        cur_func_ra_offset = AllocStackSpace(4);
    }
    if(need_fp){
        use_fp = true;
        fp_offset = AllocStackSpace(4);
    }

    for(size_t i = 0; i < func->params.len; i++){
        koopa_raw_value_t param = (koopa_raw_value_t) func->params.buffer[i];
        stack_map[param] = AllocStackSpace(4); // 参数全按 4 字节指针/整数分配
    }

    // 第二轮扫描计算并分配局部变量和数组内存
    for(size_t i = 0; i < func->bbs.len; i++){
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        for(size_t j = 0; j < bb->insts.len ;j++){
            koopa_raw_value_t inst = (koopa_raw_value_t) bb->insts.buffer[j];
            if(inst->ty->tag != KOOPA_RTT_UNIT){
                if (inst->kind.tag == KOOPA_RVT_ALLOC) {
                    stack_map[inst] = AllocStackSpace(GetTypeSize(inst->ty->data.pointer.base));
                } else {
                    stack_map[inst] = AllocStackSpace(4);
                }
            }
        }
    }

    current_stack_frame_size = ((current_stack_offset + 15) / 16) * 16;
    if(current_stack_frame_size > 0){
        if (current_stack_frame_size >= 2048) {
            cout << "\tli t4, " << current_stack_frame_size << endl;
            cout << "\tsub sp, sp, t4" << endl;
        } else {
            cout << "\taddi sp, sp, -" << current_stack_frame_size << endl;
        }
    }

    if (cur_func_need_save_ra) {
        store_value("ra", cur_func_ra_offset);
    }

    if(use_fp){
        store_value("s0", fp_offset);
        if (current_stack_frame_size >= 2048) {
            cout << "\tli t4, " << current_stack_frame_size << endl;
            cout << "\tadd s0, sp, t4" << endl;
        } else {
            cout << "\taddi s0, sp, " << current_stack_frame_size << endl;
        }
    }

    size_t reg_param_count = (func->params.len > 8) ? 8 : func->params.len;
    for (size_t i = 0; i < reg_param_count; i++) {
        int offset = stack_map[(koopa_raw_value_t)func->params.buffer[i]];
        store_value("a" + to_string(i), offset);
    }
    
    for (size_t i = 8; i < func->params.len; i++) {
        int my_offset = stack_map[(koopa_raw_value_t)func->params.buffer[i]];
        int caller_offset = (i - 8) * 4;  
        if (caller_offset >= -2048 && caller_offset <= 2047) {
            cout << "\tlw t0, " << caller_offset << "(s0)" << endl;
        } else {
            cout << "\tli t4, " << caller_offset << endl;
            cout << "\tadd t4, s0, t4" << endl;
            cout << "\tlw t0, 0(t4)" << endl;
        }
        store_value("t0", my_offset);
    }

    for(size_t i = 0; i < func->bbs.len; i++){
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        Visit(bb);
    }
}

void AsmGenerator::Visit(const koopa_raw_basic_block_t &bb){
    string label = GetBasicBlockLabel(bb);
    if (!label.empty()) cout << label << ":" << endl;
    for(size_t i = 0; i < bb->insts.len ;i++){
        koopa_raw_value_t insts = (koopa_raw_value_t) bb->insts.buffer[i];
        Visit(insts);
    }
}

void AsmGenerator::Visit(const koopa_raw_value_t &val){
    const auto& kind = val->kind;;
    switch(kind.tag){
        case KOOPA_RVT_INTEGER: break;
        case KOOPA_RVT_RETURN: Visit(kind.data.ret); break;
        case KOOPA_RVT_BINARY: Visit(val, kind.data.binary); break;
        case KOOPA_RVT_LOAD: Visit(val, kind.data.load); break;
        case KOOPA_RVT_STORE: Visit(val, kind.data.store); break;
        case KOOPA_RVT_ALLOC: break;
        case KOOPA_RVT_GLOBAL_ALLOC: Visit(val, kind.data.global_alloc); break;
        case KOOPA_RVT_BRANCH: Visit(val,kind.data.branch); break;
        case KOOPA_RVT_JUMP: Visit(val,kind.data.jump); break;
        case KOOPA_RVT_CALL: Visit(val,kind.data.call); break;
        case KOOPA_RVT_GET_PTR: Visit(val, kind.data.get_ptr); break;
        case KOOPA_RVT_GET_ELEM_PTR: Visit(val, kind.data.get_elem_ptr); break;
        default: assert(false);
    }
}

void AsmGenerator::Visit(const koopa_raw_return_t &ret){
    if(ret.value != nullptr){
        load_value(ret.value, "a0", 0);
    }
    
    if(cur_func_need_save_ra){
        if (cur_func_ra_offset >= -2048 && cur_func_ra_offset <= 2047) {
            cout << "\tlw ra, " << cur_func_ra_offset << "(sp)" << endl; 
        } else {
            cout << "\tli t4, " << cur_func_ra_offset << endl;
            cout << "\tadd t4, sp, t4" << endl;
            cout << "\tlw ra, 0(t4)" << endl;
        }
    }

    if(use_fp){
        if (fp_offset >= -2048 && fp_offset <= 2047) {
            cout << "\tlw s0, " << fp_offset << "(sp)" << endl;
        } else {
            cout << "\tli t4, " << fp_offset << endl;
            cout << "\tadd t4, sp, t4" << endl;
            cout << "\tlw s0, 0(t4)" << endl;
        }
    }

    if (current_stack_frame_size > 0) {
        if (current_stack_frame_size >= 2048) {
            cout << "\tli t4, " << current_stack_frame_size << endl;
            cout << "\tadd sp, sp, t4" << endl;
        } else {
            cout << "\taddi sp, sp, " << current_stack_frame_size << endl;
        }
    }
    cout << "\tret" << endl;
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_binary_t &binary){
    load_value(binary.lhs, "t0", 0);
    load_value(binary.rhs, "t1", 0);

    switch(binary.op){
        case KOOPA_RBO_ADD: cout << "\tadd t0, t0, t1" << endl; break;
        case KOOPA_RBO_SUB: cout << "\tsub t0, t0, t1" << endl; break;
        case KOOPA_RBO_MUL: cout << "\tmul t0, t0, t1" << endl; break;
        case KOOPA_RBO_DIV: cout << "\tdiv t0, t0, t1" << endl; break;
        case KOOPA_RBO_MOD: cout << "\trem t0, t0, t1" << endl; break;
        case KOOPA_RBO_AND: cout << "\tand t0, t0, t1" << endl; break;
        case KOOPA_RBO_OR:  cout << "\tor t0, t0, t1" << endl; break;
        case KOOPA_RBO_XOR: cout << "\txor t0, t0, t1" << endl; break;
        case KOOPA_RBO_SHL: cout << "\tsll t0, t0, t1" << endl; break;
        case KOOPA_RBO_SHR: cout << "\tsrl t0, t0, t1" << endl; break;
        case KOOPA_RBO_SAR: cout << "\tsra t0, t0, t1" << endl; break;
        case KOOPA_RBO_EQ: cout << "\txor t0, t0, t1\n\tseqz t0, t0" << endl; break;
        case KOOPA_RBO_NOT_EQ: cout << "\txor t0, t0, t1\n\tsnez t0, t0" << endl; break;
        case KOOPA_RBO_LT: cout << "\tslt t0, t0, t1" << endl; break;
        case KOOPA_RBO_GT: cout << "\tsgt t0, t0, t1" << endl; break;
        case KOOPA_RBO_LE: cout << "\tsgt t0, t0, t1\n\txori t0, t0, 1" << endl; break;
        case KOOPA_RBO_GE: cout << "\tslt t0, t0, t1\n\txori t0, t0, 1" << endl; break;
        default: assert(false && "未实现的二元操作");
    }
    store_value("t0", stack_map[val]);
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_load_t &load){
    load_value(load.src, "t0", 0); // 无论是局部地址还是全局 la，全被 load_value 打通提取到 t0
    cout << "\tlw t0, 0(t0)" << endl; 
    store_value("t0", stack_map[val]);
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_store_t &store){
    load_value(store.value, "t0", 0);
    load_value(store.dest, "t1", 0); 
    cout << "\tsw t0, 0(t1)" << endl;
}

// 数组偏移量提取：面向普通的降维数组访问
void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_get_elem_ptr_t &get_elem_ptr) {
    load_value(get_elem_ptr.src, "t0", 0); 
    load_value(get_elem_ptr.index, "t1", 0); 
    
    int elem_size = GetTypeSize(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
    cout << "\tli t2, " << elem_size << endl;
    cout << "\tmul t1, t1, t2" << endl;
    cout << "\tadd t0, t0, t1" << endl;
    store_value("t0", stack_map[val]);
}

// 数组偏移量提取：面向指针的第一维计算 (函数传进来的数组衰变)
void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_get_ptr_t &get_ptr) {
    load_value(get_ptr.src, "t0", 0);
    load_value(get_ptr.index, "t1", 0);
    
    int elem_size = GetTypeSize(get_ptr.src->ty->data.pointer.base);
    cout << "\tli t2, " << elem_size << endl;
    cout << "\tmul t1, t1, t2" << endl;
    cout << "\tadd t0, t0, t1" << endl;
    store_value("t0", stack_map[val]);
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_branch_t& branch){
    load_value(branch.cond, "t0", 0);
    string true_label = GetBasicBlockLabel(branch.true_bb);
    string false_label = GetBasicBlockLabel(branch.false_bb);
    cout << "\tbnez t0, " << true_label << endl;
    cout << "\tj " << false_label << endl;
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_jump_t& jump){
    string target_label = GetBasicBlockLabel(jump.target);
    cout << "\tj " << target_label << endl;
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_call_t& call){
    int stack_arg_count = (call.args.len > 8) ? (call.args.len - 8) : 0;
    int spill_space = stack_arg_count * 4;
    
    if (spill_space > 0) {
        if (spill_space >= 2048) {
            cout << "\tli t4, " << spill_space << endl;
            cout << "\tsub sp, sp, t4" << endl;
        } else {
            cout << "\taddi sp, sp, -" << spill_space << endl;
        }
    }
    
    for (size_t i = 0; i < call.args.len && i < 8; i++) {
        koopa_raw_value_t arg = (koopa_raw_value_t)call.args.buffer[i];
        load_value(arg, "a" + to_string(i), spill_space);
    }
    
    for (size_t i = 8; i < call.args.len; i++) {
        koopa_raw_value_t arg = (koopa_raw_value_t)call.args.buffer[i];
        load_value(arg, "t0", spill_space);
        int offset = (i - 8) * 4;
        if (offset >= -2048 && offset <= 2047) {
            cout << "\tsw t0, " << offset << "(sp)" << endl;
        } else {
            cout << "\tli t4, " << offset << endl;
            cout << "\tadd t4, sp, t4" << endl;
            cout << "\tsw t0, 0(t4)" << endl;
        }
    }
    
    string callee_name = call.callee->name + 1;
    cout << "\tcall " << callee_name << endl;
    
    if (spill_space > 0) {
        if (spill_space >= 2048) {
            cout << "\tli t4, " << spill_space << endl;
            cout << "\tadd sp, sp, t4" << endl;
        } else {
            cout << "\taddi sp, sp, " << spill_space << endl;
        }
    }
    
    if (val->ty->tag != KOOPA_RTT_UNIT) {
        store_value("a0", stack_map[val]);
    }
}

// 全局变量定义 (标量及多维数组)
void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_global_alloc_t& global_alloc){
    string name = val->name + 1;
    koopa_raw_value_t init = global_alloc.init;
    
    cout << "\t.data" << endl;
    cout << "\t.globl " << name << endl;
    cout << name << ":" << endl;
    
    if (init->kind.tag == KOOPA_RVT_INTEGER) {
        cout << "\t.word " << init->kind.data.integer.value << endl;
    } else if (init->kind.tag == KOOPA_RVT_ZERO_INIT) {
        int size = GetTypeSize(val->ty->data.pointer.base);
        cout << "\t.zero " << size << endl;
    } else if (init->kind.tag == KOOPA_RVT_AGGREGATE) {
        VisitAggregate(init);
    }
}

// 递归遍历打印多维数组大括号的实际数据
void AsmGenerator::VisitAggregate(koopa_raw_value_t init) {
    if (init->kind.tag == KOOPA_RVT_INTEGER) {
        cout << "\t.word " << init->kind.data.integer.value << endl;
    } else if (init->kind.tag == KOOPA_RVT_AGGREGATE) {
        koopa_raw_slice_t elems = init->kind.data.aggregate.elems;
        for (size_t i = 0; i < elems.len; ++i) {
            VisitAggregate((koopa_raw_value_t)elems.buffer[i]);
        }
    }
}