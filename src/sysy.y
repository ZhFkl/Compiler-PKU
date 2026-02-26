%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }


%union {
  std::string *str_val;
  int int_val;
  BaseAST* ast_val;
}

%token INT RETURN LE GE EQ NEQ LAND LOR CONST IF ELSE
%token <str_val> IDENT 
%token <int_val> INT_CONST


%type <ast_val> FuncDef FuncType Block Stmt 
RelExp EqExp LAndExp LOrExp Exp PrimaryExp 
UnaryExp Number AddExp MulExp Decl ConstDecl
BType ConstDef ConstInitVal BlockItem BlockItemList
LVal ConstExp ConstDefList VarDecl VarDef VarDefList
InitVal


%type <int_val> UnaryOp AddOp MulOp 
%type <str_val> RelOp EqOp

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = std::move(comp_unit);
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    $$ = ast;
  }
  ;

Block: '{' BlockItemList '}' {
  $$ = $2;
}
| '{' '}'{
  $$ = new BlockAST();
};

BlockItemList: BlockItem {
    auto ast = new BlockAST();
    ast->block_items.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | BlockItemList BlockItem {
    auto ast = static_cast<BlockAST*>($1);
    ast->block_items.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  };

BlockItem: Decl{
  auto ast = new BlockItemAST();
  ast->decl = unique_ptr<BaseAST>($1);
  $$ = ast;
}
| Stmt {
  auto ast = new BlockItemAST();
  ast->stmt = unique_ptr<BaseAST>($1);
  $$ = ast;
};


Stmt
  : LVal '=' Exp ';' {
      // 匹配: LVal = Exp;
      auto ast = new StmtAST();
      ast->lval = unique_ptr<BaseAST>($1);
      ast->exp = unique_ptr<BaseAST>($3);
      $$ = ast;
  }
  | Exp ';' {
      // 匹配: Exp;
      auto ast = new StmtAST();
      ast->exp = unique_ptr<BaseAST>($1);
      $$ = ast;
  }
  | ';' {
      // 匹配: ; (空语句)
      $$ = new StmtAST(); // 内部所有指针全为空，is_return 也是 false
  }
  | Block {
      // 匹配: Block
      auto ast = new StmtAST();
      ast->block = unique_ptr<BaseAST>($1);
      $$ = ast;
  }
  | RETURN Exp ';' {
      // 匹配: return Exp;
      auto ast = new StmtAST();
      ast->is_return = true;
      ast->exp = unique_ptr<BaseAST>($2);
      $$ = ast;
  }
  | RETURN ';' {
      // 匹配: return;
      auto ast = new StmtAST();
      ast->is_return = true;
      $$ = ast;
  }
  | IF '(' Exp ')' Stmt{
      auto ast = new StmtAST();
      ast->is_if = true;
      ast->cond = unique_ptr<BaseAST>($3);
      ast->then_stmt = unique_ptr<BaseAST>($5);
      $$ = ast;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt{
      auto ast = new StmtAST();
      ast->is_if = true;
      ast->cond = unique_ptr<BaseAST>($3);
      ast->then_stmt = unique_ptr<BaseAST>($5);
      ast->else_stmt = unique_ptr<BaseAST>($7);
      $$ = ast;
  };


Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number = unique_ptr<BaseAST>($1);
    $$ = ast;
  }| LVal {
    auto ast = new PrimaryExpAST();
    ast->LVal = unique_ptr<BaseAST>($1);
    $$ = ast;
  }


UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp{
    auto ast = new UnaryExpAST();
    ast->op = $1;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

UnaryOp
 : '+' { $$ = '+';}
 | '-' { $$ = '-';}
 | '!' { $$ = '!';}
 ;

AddOp
  : '+' { $$ = '+';}
  | '-' { $$ = '-';}
  ;

MulOp
  : '*' { $$ = '*';}
  | '/' { $$ = '/';}
  | '%' { $$ = '%';}
  ;




Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->value = $1;
    $$ = ast;
  }
  ;

LVal : IDENT {
  auto ast = new LValAST();
  ast->ident = *$1;
  $$ = ast;
}

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp{
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->op = $2;
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp 
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp{
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->op = $2;
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelOp
  : '<' { $$ = new std::string("<"); }
  | '>' { $$ = new std::string(">"); }
  | LE  { $$ = new std::string("<="); }
  | GE  { $$ = new std::string(">="); }
  ;

EqOp
  : EQ  { $$ = new std::string("=="); }
  | NEQ { $$ = new std::string("!="); }
  ;


RelExp
  : AddExp {
    auto ast = new RelExp();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExp();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);     
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExp();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EqOp RelExp {
    auto ast = new EqExp();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<string>($2);      
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExp();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExp();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExp();
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOrExp();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl{
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl{
    auto ast = new DeclAST();
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';'{
    auto ast = static_cast<ConstDeclAST*>($3);
    ast->b_type = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto ast = new ConstDeclAST();
    ast->const_defs.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstDefList ',' ConstDef {
    auto ast = static_cast<ConstDeclAST*>($1);
    ast->const_defs.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

BType
  : INT{
    auto ast = new BTypeAST();
    $$ = ast;
  }
  ;

ConstDef : IDENT '=' ConstInitVal{
  auto ast = new ConstDefAST();
  ast->ident = *$1;
  ast->const_init_val = unique_ptr<BaseAST>($3);
  $$ = ast;
}


ConstInitVal: ConstExp{
  auto ast = new ConstInitValAST();
  ast->const_exp = unique_ptr<BaseAST>($1);
  $$ = ast;
};

ConstExp : Exp{
  auto ast = new ConstExpAST();
  ast->exp = unique_ptr<BaseAST>($1);
  $$ = ast;
};

VarDecl: BType VarDefList ';'{
    auto ast = static_cast<VarDeclAST*>($2);
    ast->b_type = unique_ptr<BaseAST>($1);
    $$ = ast;
};

VarDefList : VarDef {
    auto ast = new VarDeclAST();
    ast->var_defs.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | VarDefList ',' VarDef {
    auto ast = static_cast<VarDeclAST*>($1);
    ast->var_defs.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  };

VarDef: IDENT{
  auto ast = new VarDefAST();
  ast->ident = *$1;
  $$ = ast;
} | IDENT '=' InitVal{
  auto ast = new VarDefAST();
  ast->ident = *$1;
  ast->init_val = unique_ptr<BaseAST>($3);
  $$ = ast;
};

InitVal: Exp{
  auto ast = new InitValAST();
  ast->exp = unique_ptr<BaseAST>($1);
  $$ = ast;
};


//pa5


%%

void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
