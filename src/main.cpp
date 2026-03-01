#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include "ast.h"
#include "visit.h"
using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
//函数声明

int main(int argc, const char *argv[]) {
  assert(argc == 5);
    // 初始化变量
    std::string mode;        
    std::string input_file;   
    std::string output_file;  
    cout << "test" << endl; 
     // 正确解析命令行参数：-koopa input -o output
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-koopa" || arg == "-riscv") {  // 识别选项 -koopa
            mode = arg.substr(1);     // mode 赋值为 "koopa" 或 "riscv"（不带 -）
            // 下一个参数是输入文件
            if (i + 1 < argc) {
                input_file = argv[++i];
            } else {
                std::cerr << "错误：-koopa 后必须指定输入文件！" << std::endl;
                return 1;
            }
        } else if (arg == "-o") {  // 识别选项 -o
            // 下一个参数是输出文件
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "错误：-o 后必须指定输出文件！" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "错误：未知参数 " << arg << std::endl;
            return 1;
        }
    }
  
  // 打印解析结果（调试用）
  std::cout << "mode: " << mode << std::endl;
  std::cout << "input_file: " << input_file << std::endl;
  std::cout << "output_file: " << output_file << std::endl;

  //打开输入文件
  yyin = fopen(input_file.c_str(), "r");
  assert(yyin);

  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);
  fclose(yyin);

  if (ast) { // 加上这层保护！
      cout << "AST 结构" << endl;
  } else {
      cerr << "Compiler Error: Parsing failed, AST is null!" << endl;
      return 1; // 发生语法错误，返回非0状态码
  }
  cout << "test" << endl;
  string koopa_ir = ast->GenKoopaIR();
  cout << "test" << endl;
  if(mode == "koopa"){
      cout << "Koopa IR 代码" << endl;
      cout << koopa_ir;
      ofstream out(output_file);
      assert(out.is_open());
      out << koopa_ir;
      out.close();
    }else if(mode == "riscv"){
      koopa_program_t program ;
      koopa_error_code_t ret = koopa_parse_from_string(koopa_ir.c_str(),&program);
      assert(ret == KOOPA_EC_SUCCESS);


      koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
      koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
      koopa_delete_program(program);


      freopen(output_file.c_str(), "w", stdout);

        // 2. 遍历 raw 结构，开始输出汇编
      Visit(raw);

        // 恢复 stdout 并清理
      fclose(stdout);


      //处理完成释放raw program builder占用的内存
      koopa_delete_raw_program_builder(builder);
    }else{
      cout << "the output file is empty" << endl;
    }
  
  
  return 0;
}

