#ifndef ADIOS2_DERIVED_PARSER_ASTNODE_H_
#define ADIOS2_DERIVED_PARSER_ASTNODE_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "../ExprHelper.h"

/*****************************************/

namespace adios2
{
namespace detail
{

class ASTNode
{
public:
    ASTNode();
    ASTNode(ExpressionOperator);
    ASTNode(ExpressionOperator, const char *a);
    ASTNode(ExpressionOperator, double val);
    ASTNode(ExpressionOperator, ASTNode *e);
    ASTNode(ExpressionOperator, ASTNode *e, const char *i);
    ASTNode(ExpressionOperator, ASTNode *e1, ASTNode *e2);

    // Copy constructor
    ASTNode(const ASTNode &e);

    ~ASTNode();

    static std::pair<std::string, std::string> lookup_var(const std::string var_alias);
    static std::string lookup_var_path(const std::string var_alias);
    static std::string lookup_var_indices(const std::string var_alias);
    static void add_lookup_entry(const std::string alias, const std::string var_name,
                                 const std::string indices);

    void add_subexpr(ASTNode *e);
    void add_back_subexpr(ASTNode *e, size_t i);
    void extend_subexprs(size_t n);
    void infer_type();
    void printpretty(std::string indent = "");

    // private:
    ExpressionOperator operation;
    std::string alias;
    std::string indices;
    double value;
    std::vector<ASTNode *> sub_expr;

    static std::map<std::string, std::pair<std::string, std::string>> var_lookup;
};

}
}
#endif