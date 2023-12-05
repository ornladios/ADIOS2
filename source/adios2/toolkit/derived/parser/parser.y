/* calculator. */
%{
 #include <stdio.h>
 #include <stdlib.h>
 #include <math.h>
 #include <stack>
 #include <string>
 #include <vector>
 #include <iostream>
 #include "lexer.h"
 #include "ASTNode.h"
  
  extern int yyparse(std::stack<ASTNode*>* expr_stack);

  void* createExpr(std::stack<ASTNode*>*, std::string, const char*, double, size_t);
  
  static void* yyparse_value;  

  void yyerror(std::stack<ASTNode*>* expr_stack, const char *msg);

%}

%parse-param {std::stack<ASTNode*>* expr_stack}

%union{
  double dval;
  int ival;
  char* sval;
  void* expr_ptr;
}

%error-verbose
%locations

%start input
%token COMMA L_PAREN R_PAREN ENDL
%token FUNCTION
%token OPERATOR
%token INDICES
%token NUMBER
%token ALIAS PATH
%type <dval> NUMBER
%type <sval> ALIAS PATH INDICES
%type <sval> FUNCTION OPERATOR
%type <expr_ptr> input exp
%type <ival> list


%% 

input:                  {}
                        | ENDL input             {}
                        | decl input             {}
                        | exp input              { /*yyparse_value = $1->expression;*/ }
			;

decl:                   ALIAS PATH               { ASTNode::add_lookup_entry($1, $2, ""); }
                        | ALIAS PATH INDICES     { ASTNode::add_lookup_entry($1, $2, $3); }
                        ;

//index:                  NUMBER comma index { ASTNode::extend_current_lookup_indices($1); }
//                        | NUMBER { ASTNode::extend_current_lookup_indices($1); }
//                        ;

list:                   list COMMA exp { $$ = $1 +1; }
                        | exp { $$ = 1;}
                        ;

exp:                    ALIAS                  { $$ = createExpr(expr_stack, "ALIAS", $1, 0, 0); }
| ALIAS INDICES         { createExpr(expr_stack, "ALIAS", $1, 0, 0); $$ = createExpr(expr_stack, "INDEX", $2, 0, 1); }
| PATH                  { $$ = createExpr(expr_stack, "PATH", $1, 0, 0); }
| NUMBER                { $$ = createExpr(expr_stack, "NUM", "", $1, 0); }
| L_PAREN exp R_PAREN { $$ = $2; }
| exp OPERATOR exp { $$ = createExpr(expr_stack, $2, "", 0, 2); }
| FUNCTION L_PAREN list R_PAREN { $$ = createExpr(expr_stack, $1, "", 0, $3); }
			;
%%

void* createExpr(std::stack<ASTNode*>* expr_stack, std::string str_op, const char* name, double value, size_t numsubexprs) {
  std::cout << "Creating ASTNode in function createExpr" << std::endl;
  std::cout << "\tstack size: " << expr_stack->size() << "\n\top: " << str_op << "\n\tname: " << name << "\n\tvalue: " << value << "\n\tnumsubexprs: " << numsubexprs << std::endl;

  ExprHelper::expr_op op = ExprHelper::get_op(str_op);

  ASTNode *node = new ASTNode(op);
  switch(op) {
  case ExprHelper::OP_ALIAS:
    node = new ASTNode(op, name);
    break;
  case ExprHelper::OP_PATH:
    node = new ASTNode(op, name);
    break;
  case ExprHelper::OP_NUM:
    node = new ASTNode(op, value);
    break;
  case ExprHelper::OP_INDEX:
    // TODO: translate indices
    node = new ASTNode(op, name);
    break;
  default:
    node = new ASTNode(op);
  };
  node->extend_subexprs(numsubexprs);
  for (size_t i = 1; i <= numsubexprs; ++i)
    {
      ASTNode *subexpr = expr_stack->top();
      node->add_back_subexpr(subexpr,i);
      expr_stack->pop();
    }
  expr_stack->push(node);

  return &expr_stack->top();
}

Expression* parse_expression(const char* input) {
  yy_scan_string(input);
  std::stack<ASTNode*> expr_stack;
  yyparse(&expr_stack);

  // DEBUGGING
  std::cout << "yyparse complete. Stack size: " << expr_stack.size() << std::endl;
  std::cout << "parser prettyprint:" << std::endl;
  expr_stack.top()->printpretty("");

  Expression *dummy_root = new Expression();
  expr_stack.top()->to_expr(dummy_root);
  return std::get<0>(dummy_root->sub_exprs[0]);
}

void yyerror(std::stack<ASTNode*>* expr_stack, const char *msg) {
   printf("** Line %d: %s\n", yylloc.first_line, msg);
}