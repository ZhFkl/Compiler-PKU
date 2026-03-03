#include "visit.h"
#include <iostream>
#include <string>
#include <cassert>
#include <unordered_map>
using namespace std;

AsmGenerator::AsmGenerator() = default;
void AsmGenerator::Generate(const koopa_raw_program_t &program){
    Visit(program);
}


void AsmGenerator::load_value(koopa_raw_value_t val,const string&reg,int sp_offset){
    if(val->kind.tag == KOOPA_RVT_INTEGER){
       cout << "\tli " << reg << ", " << val->kind.data.integer.value << endl;
    }else {
        //此时是从栈上来的值
        assert(stack_map.find(val) != stack_map.end() && "访问了未分配的值");
       int offset = stack_map[val] + sp_offset;
        cout << "\tlw " << reg << ", " << offset << "(sp)" << endl;
    }
}


string AsmGenerator::GetBasicBlockLabel(koopa_raw_basic_block_t bb) {
    if (!bb || !bb->name) {
        // 万一遇到没有名字的匿名基本块，生成一个唯一编号
        static int anon_count = 0;
        return ".L_" + current_func_name + "_anon_" + std::to_string(anon_count++);
    }
    
    std::string name = bb->name;
    if (!name.empty() && name[0] == '%') {
        name = name.substr(1);  // 去掉 '%'
    }
    
    // 拼接成局部标签： .L_函数名_块名
    return ".L_" + current_func_name + "_" + name;
}

bool AsmGenerator::HasCallINFunc(const koopa_raw_function_t &func){
    for(size_t i = 0; i < func->bbs.len; i++){
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        for(size_t j = 0; j < bb->insts.len ;j++){
            koopa_raw_value_t insts = (koopa_raw_value_t) bb->insts.buffer[j];
            if(insts->kind.tag == KOOPA_RVT_CALL){
                return true;
            }
        }
    }
    return false;
}
int AsmGenerator::AllocStackSpace(int size){
    
    int offset = current_stack_offset;
    current_stack_offset += size;
    //cout << "alloc " << current_stack_offset << endl;
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

void AsmGenerator::Visit(const koopa_raw_function_t & func){
    if(func->bbs.len == 0) return;
    string name = func->name + 1;
    current_func_name = name;
    cout << "\t.text" << endl;
    cout << "\t.globl " << name << endl;
    cout << name << ":" << endl;


    //栈分配空间
    stack_map.clear();
    current_stack_offset = 0;//此时的current_stack offset 是为了之后的栈对齐
    use_fp = false;
    fp_offset = 0;


    cur_func_need_save_ra = HasCallINFunc(func);
    cur_func_ra_offset = -1;


    bool need_fp = (func->params.len > 8); 

    //预留此时的ra位置
    if (cur_func_need_save_ra) {
        cur_func_ra_offset = AllocStackSpace(4);
    }

    //如果此时需要栈帧的预留s0的位置
    if(need_fp){
        use_fp = true;
        fp_offset = AllocStackSpace(4);
    }
    //为此时的参数分配空间
    for(size_t i = 0; i < func->params.len; i++){
        koopa_raw_value_t param = (koopa_raw_value_t) func->params.buffer[i];
        stack_map[param] = AllocStackSpace(4);
    }



    for(size_t i = 0; i < func->bbs.len; i++){
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        for(size_t j = 0; j < bb->insts.len ;j++){
            koopa_raw_value_t insts = (koopa_raw_value_t) bb->insts.buffer[j];
            if(insts->ty->tag != KOOPA_RTT_UNIT){
                stack_map[insts] = AllocStackSpace(4);
            }
        }
    }
    //计算对其的栈指针

    current_stack_frame_size = ((current_stack_offset + 15) / 16) * 16;
    if(current_stack_frame_size > 0){
        //分配栈空间
        cout << "\taddi sp, sp, -" << current_stack_frame_size << endl;
    }

    if (cur_func_need_save_ra) {
        cout << "\tsw ra, " << cur_func_ra_offset << "(sp)" << endl;
    }


    if(use_fp){
        cout << "\tsw s0, " << fp_offset << "(sp)" << endl;
        cout << "\taddi s0, sp, " << current_stack_frame_size << endl;
    }

    size_t reg_param_count = (func->params.len > 8) ? 8 : func->params.len;


    for (size_t i = 0; i < reg_param_count; i++) {
        int offset = stack_map[(koopa_raw_value_t)func->params.buffer[i]];
        cout << "\tsw a" << i << ", " << offset << "(sp)" << endl;
    }
    
    // 第9个及以后：从Caller的栈加载，保存到自己的栈
    for (size_t i = 8; i < func->params.len; i++) {
        int my_offset = stack_map[(koopa_raw_value_t)func->params.buffer[i]];
        int caller_offset = (i - 8) * 4;  // 在Caller栈中的位置：0, 4, 8, ...
        
        cout << "\tlw t0, " << caller_offset << "(s0)" << endl;
        cout << "\tsw t0, " << my_offset << "(sp)" << endl;
    }

    //栈空间分配完毕开始执行block解析
    for(size_t i = 0; i < func->bbs.len; i++){
        assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);
        koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
        Visit(bb);
    }

}



void AsmGenerator::Visit(const koopa_raw_basic_block_t &bb){
    string label = GetBasicBlockLabel(bb);
    if (!label.empty()) {
        cout << label << ":" << endl;
    }
    for(size_t i = 0; i < bb->insts.len ;i++){
        assert(bb->insts.kind == KOOPA_RSIK_VALUE);
        koopa_raw_value_t insts = (koopa_raw_value_t) bb->insts.buffer[i];
        Visit(insts);
    }
}



void AsmGenerator::Visit(const koopa_raw_value_t &val){
    const auto& kind = val->kind;;
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
            Visit(val, kind.data.global_alloc);
            break;
        case KOOPA_RVT_BRANCH:
            Visit(val,kind.data.branch);
            break;
        case KOOPA_RVT_JUMP:
            Visit(val,kind.data.jump);
            break;
        case KOOPA_RVT_CALL:
            Visit(val,kind.data.call);
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



void AsmGenerator::Visit(const koopa_raw_return_t &ret){
    // 返回值需要被放入 a0 寄存器
    if(ret.value != nullptr){
        load_value(ret.value, "a0",0);
    }
    
    if(cur_func_need_save_ra){
        cout << "\tlw ra, " << cur_func_ra_offset << "(sp)" << endl; 
    }

    // 恢复栈指针 (与函数开头的分配对称)
    if (current_stack_frame_size > 0) {
        cout << "\taddi sp, sp, " << current_stack_frame_size << endl;
    }
    
    // 生成 ret 指令
    cout << "\tret" << endl;
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_binary_t &binary){
    //先加载到我的寄存器中
    load_value(binary.lhs, "t0",0);
    load_value(binary.rhs, "t1",0);

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


void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_load_t &load){
   if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        string name = load.src->name + 1; // 去掉 '@'
        cout << "\tla t0, " << name << endl;  // 将全局变量的绝对地址加载到 t0
        cout << "\tlw t0, 0(t0)" << endl;     // 从该地址读取数据存入 t0
    } else {
        // 否则就是栈上的局部变量，走原来的逻辑
        int offset = stack_map[load.src];
        cout << "\tlw t0, " << offset << "(sp)" << endl;
    }
    
    // 把读取到的值保存到当前 %0, %1 对应的栈空间中
    int val_offset = stack_map[val];
    cout << "\tsw t0, " << val_offset << "(sp)" << endl;
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_store_t &store){
    load_value(store.value, "t0",0);
    
    // 判断目的地是不是全局变量
    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        string name = store.dest->name + 1; // 去掉 '@'
        cout << "\tla t1, " << name << endl;  // 把全局变量的地址加载到 t1 寄存器
        cout << "\tsw t0, 0(t1)" << endl;     // 将 t0 的值存入 t1 指向的内存
    } else {
        // 局部变量，存入对应的栈偏移
        int offset = stack_map[store.dest];
        cout << "\tsw t0, " << offset << "(sp)" << endl;
    }
}

 void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_branch_t& branch){
    load_value(branch.cond, "t0",0 );
    string true_label = GetBasicBlockLabel(branch.true_bb);
    string false_label = GetBasicBlockLabel(branch.false_bb);
    cout << "\tbnez t0, " << true_label << endl;
    cout << "\tj " << false_label << endl;
 }

 void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_jump_t& jump){
    //cout << "test"<< endl;
    string target_label = GetBasicBlockLabel(jump.target);
    cout << "\tj " << target_label << std::endl;
 }

 void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_call_t& call){
    int stack_arg_count = (call.args.len > 8) ? (call.args.len - 8) : 0;
    int spill_space = stack_arg_count * 4;
    
    // 临时分配栈空间给溢出参数
    if (spill_space > 0) {
        cout << "\taddi sp, sp, -" << spill_space << endl;
    }
    
    // 前8个参数：加载到寄存器 a0-a7
    for (size_t i = 0; i < call.args.len && i < 8; i++) {
        koopa_raw_value_t arg = (koopa_raw_value_t)call.args.buffer[i];
        load_value(arg, "a" + to_string(i),spill_space);
    }
    
    // 第9个及以后：存到Caller的栈（临时空间）
    for (size_t i = 8; i < call.args.len; i++) {
        koopa_raw_value_t arg = (koopa_raw_value_t)call.args.buffer[i];
        load_value(arg, "t0", spill_space);
        cout << "\tsw t0, " << (i - 8) * 4 << "(sp)" << endl;
    }
    
    // 调用
    string callee_name = call.callee->name + 1;
    cout << "\tcall " << callee_name << endl;
    
    // 恢复栈
    if (spill_space > 0) {
        cout << "\taddi sp, sp, " << spill_space << endl;
    }
    
    // 保存返回值
    if (val->ty->tag != KOOPA_RTT_UNIT) {
        int offset = stack_map[val];  // 直接查 map
        cout << "\tsw a0, " << offset << "(sp)" << endl;
    }
}

void AsmGenerator::Visit(const koopa_raw_value_t &val, const koopa_raw_global_alloc_t& global_alloc){
    //全局变量的分配
    string name = val->name + 1;
    koopa_raw_value_t init = global_alloc.init;
    if(init->kind.tag == KOOPA_RVT_INTEGER){
        cout << "\t.data" << endl;
        cout << "\t.globl " << name << endl;
        cout << name << ":" << endl;
        cout << "\t.word " << init->kind.data.integer.value << endl;
    }else if(init->kind.tag == KOOPA_RVT_ZERO_INIT){
        cout << "\t.data" << endl;
        cout << "\t.globl " << name << endl;
        cout << name << ":" << endl;
        cout << "\t.zero 4" << endl; // 假设全局变量占4字节
    }
}