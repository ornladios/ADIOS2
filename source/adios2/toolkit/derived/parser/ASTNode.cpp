#ifndef ADIOS2_DERIVED_PARSER_ASTNODE_CPP_
#define ADIOS2_DERIVED_PARSER_ASTNODE_CPP_

#include "ASTNode.h"

namespace adios2
{
namespace detail
{

/*****************************************/
// alias maps to pair of path and indices (indices may be empty string)
std::map<std::string, std::pair<std::string, std::string>> ASTNode::var_lookup;

ASTNode::ASTNode() {}

ASTNode::ASTNode(ExpressionOperator op) : operation(op) {}

ASTNode::ASTNode(ExpressionOperator op, const char *str) : operation(op)
{
    switch (operation)
    {
    case ExpressionOperator::OP_ALIAS:
        alias = str;
        break;
    case ExpressionOperator::OP_PATH:
        alias = str;
        break;
    case ExpressionOperator::OP_INDEX:
        indices = str;
        break;
    default:
        // TODO: Make some error
        // std::cout << "***That's a problem... ASTNode constructed with string should be alias
        // type, path type,  or index type\n";
        break;
    }
}

ASTNode::ASTNode(ExpressionOperator op, double val) : operation(op), value(val) {}

ASTNode::ASTNode(ExpressionOperator op, ASTNode *e) : operation(op) { sub_expr.push_back(e); }

// for index
ASTNode::ASTNode(ExpressionOperator op, ASTNode *e, const char *str) : operation(op), indices(str)
{
    sub_expr.push_back(e);
}

ASTNode::ASTNode(ExpressionOperator op, ASTNode *e1, ASTNode *e2) : operation(op)
{
    sub_expr.push_back(e1);
    sub_expr.push_back(e2);
}

// Copy constructor
ASTNode::ASTNode(const ASTNode &e)
: operation(e.operation), alias(e.alias), value(e.value), sub_expr(e.sub_expr)
{
}

ASTNode::~ASTNode()
{
    for (ASTNode *e : sub_expr)
    {
        delete e;
    }
}

std::pair<std::string, std::string> ASTNode::lookup_var(const std::string var_alias)
{
    return var_lookup[var_alias];
}

std::string ASTNode::lookup_var_path(const std::string var_alias)
{
    return var_lookup[var_alias].first;
}

std::string ASTNode::lookup_var_indices(const std::string var_alias)
{
    return var_lookup[var_alias].second;
}

void ASTNode::add_lookup_entry(const std::string alias, const std::string var_name,
                               const std::string indices)
{
    // std::cout << "Adding alias to lookup table:\n\talias: " << alias << "\n\tvar_name: " <<
    // var_name << "\n\tindices: " << indices <<  std::endl;
    var_lookup[alias] = std::make_pair(var_name, indices);
}

void ASTNode::add_subexpr(ASTNode *e) { sub_expr.push_back(e); }

void ASTNode::add_back_subexpr(ASTNode *e, size_t n)
{
    size_t index = sub_expr.size() - n;
    // std::cout << "ASTNode add_back_subexpr index: " << index << std::endl;
    // if (index > 0 && sub_expr[index] == nullptr)
    sub_expr[index] = e;
}

void ASTNode::extend_subexprs(size_t n)
{
    // std::cout << "ASTNode extending subexprs from size " << sub_expr.size() << " to " <<
    // (sub_expr.size() + n) << std::endl;
    sub_expr.resize(sub_expr.size() + n);
}

void ASTNode::printpretty(std::string indent)
{
    std::cout << indent << get_op_name(operation) << ":";
    if (operation == ExpressionOperator::OP_ALIAS)
    {
        std::cout << " (alias " << alias << " maps to Variable '";
        std::cout << lookup_var_path(alias) << "'";
        if (lookup_var_indices(alias) != "")
        {
            std::cout << " [" << lookup_var_indices(alias) << "]";
        }
        std::cout << ")";
    }
    else if (operation == ExpressionOperator::OP_PATH)
    {
        std::cout << " (" << alias << ")";
    }
    else if (operation == ExpressionOperator::OP_INDEX)
    {
        std::cout << " [" << indices << "]";
    }
    std::cout << std::endl;
    for (ASTNode *e : sub_expr)
    {
        if (e != nullptr)
            e->printpretty(indent + "    ");
        else
            std::cout << "sub_expr is nullptr" << std::endl;
    }
}

}
}
#endif
