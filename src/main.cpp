#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include "ast.h"
using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {

  assert(argc == 5);
    // 初始化变量
    std::string mode;        
    std::string input_file;   
    std::string output_file;  

    // 正确解析命令行参数：-koopa input -o output
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-koopa") {  // 识别选项 -koopa
            mode = "koopa";     // mode 赋值为 "koopa"（不带 -）
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

  cout << "AST 结构" << endl;
  ast->Dump();
  cout << endl;


  if(mode == "koopa"){
      string koopa_ir = ast->GenKoopaIR();
      ofstream out(output_file);
      assert(out.is_open());
      out << koopa_ir;
      out.close();
    }else{
      cout << "the output file is empty" << endl;
    }
  
  
  return 0;
}
