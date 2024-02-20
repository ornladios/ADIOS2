#ifndef ASTDRIVER_HH_
# define ASTDRIVER_HH_

#include <string>
#include <tuple>
#include <stack>
#include <map>
#include "parser.h"
#include "ASTNode.h"

# define YY_DECL \
  adios2::detail::parser::symbol_type yylex (ASTDriver& drv)
YY_DECL;

using indx_type = std::vector<std::tuple<int, int, int>>;

class ASTDriver
{
public:
  ASTDriver ();

  // Defined in lexer.l
  ASTNode *parse (const char* input);

  ASTNode* getAST ();

  void resolve (ASTNode *node);

  std::tuple<std::string, indx_type> lookup_var(const std::string alias);
  std::string lookup_var_name(const std::string alias);
  indx_type lookup_var_indices(const std::string alias);

  void add_lookup_entry(std::string alias, std::string var_name, indx_type indices);
  void add_lookup_entry(std::string alias, std::string var_name);

  void createNode(std::string, size_t);
  void createNode(std::string);
  void createNode(std::string, indx_type);
  
  // Whether to generate parser debug traces.
  bool trace_parsing;
  // Whether to generate scanner debug traces.
  bool trace_scanning;
  // The token's location used by the scanner.
  adios2::detail::location location;

private:
  ASTNode* root;

  // count of how many ASTNodes exist
  size_t num_nodes;

  // While parsing, holds ASTNodes until parent node is created
  // (since root node is created last from bottom up design)
  std::stack<ASTNode*> holding;
  
  // map alias names to variable names and indices from alias definition
  std::map<std::string, std::tuple<std::string, indx_type>> aliases;
};
#endif // ! ASTDRIVER_HH_
