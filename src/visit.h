#pragma once 
#include "koopa.h"
#include <string>
#include <unordered_map>
using namespace std;
class AsmGenerator {
public:
    AsmGenerator();
    void Generate(const koopa_raw_program_t &program);
private:
    string current_func_name;
    std:: unordered_map<koopa_raw_value_t, int> stack_map;
    int current_stack_frame_size = 0;
    int current_stack_offset = 0;

    bool cur_func_need_save_ra = false;
    int cur_func_ra_offset = 0;

    bool use_fp = false;
    int fp_offset = 0;

    bool HasCallINFunc(const koopa_raw_function_t &func);
    int AllocStackSpace(int size);
    void load_value(koopa_raw_value_t val,const std::string&reg,int sp_offset);
    string GetBasicBlockLabel(koopa_raw_basic_block_t bb);


    void Visit(const koopa_raw_program_t &program);
    void Visit(const koopa_raw_slice_t &slice);
    void Visit(const koopa_raw_function_t & func);
    void Visit(const koopa_raw_basic_block_t &bb);
    void Visit(const koopa_raw_value_t &val);
    void Visit(const koopa_raw_return_t &ret);
    void Visit(const koopa_raw_value_t &val, const koopa_raw_binary_t &binary);    
    void Visit(const koopa_raw_value_t &val, const koopa_raw_load_t &load);
    void Visit(const koopa_raw_value_t &val, const koopa_raw_store_t &store);
    void Visit(const koopa_raw_value_t &val, const koopa_raw_branch_t& branch);
    void Visit(const koopa_raw_value_t &val, const koopa_raw_jump_t& jump);
    void Visit(const koopa_raw_value_t &val, const koopa_raw_call_t& call);
    void Visit(const koopa_raw_value_t &val, const koopa_raw_global_alloc_t& global_alloc);
    void SaveParamToStack(const koopa_raw_function_t &func);
};
