#pragma once
#include "koopa.h"

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t & func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &val);
//void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_value_t &val, const koopa_raw_binary_t &binary);
void Visit(const koopa_raw_value_t &val, const koopa_raw_load_t &load);
void Visit(const koopa_raw_value_t &val, const koopa_raw_store_t &store);