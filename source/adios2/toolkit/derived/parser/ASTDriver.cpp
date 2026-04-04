/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ASTDriver.h"

namespace adios2
{
namespace detail
{

using indx_type = std::vector<std::tuple<int, int, int>>;

ASTDriver::ASTDriver() {}

ASTDriver::ASTDriver(const std::string input) { ASTDriver::parse(input); }

ASTDriver::~ASTDriver()
{
    ASTDriver::destroy_lex_structures();
    while (holding.size() > 0)
    {
        delete holding.top();
        holding.pop();
    }
}

ASTNode *ASTDriver::getAST()
{
    if (hasError)
        throw std::invalid_argument("Failed to parse derived expression: " + errorMessage);
    if (holding.size() == 0)
        throw std::invalid_argument("Derived expression is empty");
    resolve(holding.top());
    return holding.top();
}

void ASTDriver::resolve(ASTNode *node)
{
    if (!node->get_alias().empty())
    {
        auto it = aliases.find(node->get_alias());
        if (it != aliases.end())
        {
            node->set_varname(std::get<0>(it->second));
            node->set_indices(std::get<1>(it->second));
        }
        else
        {
            // No alias defined — treat the name as a direct variable reference
            node->set_varname(node->get_alias());
        }
    }
    for (ASTNode *subexpr : node->get_subexprs())
    {
        resolve(subexpr);
    }
}

std::tuple<std::string, indx_type> ASTDriver::lookup_var(const std::string alias)
{
    return aliases[alias];
}

std::string ASTDriver::lookup_var_name(const std::string alias)
{
    std::tuple<std::string, indx_type> var = aliases[alias];
    return std::get<0>(var);
}

indx_type ASTDriver::lookup_var_indices(const std::string alias)
{
    std::tuple<std::string, indx_type> var = aliases[alias];
    return std::get<1>(var);
}

void ASTDriver::add_lookup_entry(std::string alias, std::string var_name, indx_type indices)
{
    aliases.insert({alias, {var_name, indices}});
}

void ASTDriver::add_lookup_entry(std::string alias, std::string var_name)
{
    aliases.insert({alias, {var_name, {}}});
}

void ASTDriver::add_number(double num)
{
    ASTNode *node = new ASTNode("NUM", num);
    holding.push(node);
}

void ASTDriver::createNode(std::string op_name, size_t numsubexprs)
{
    ASTNode *node = new ASTNode(op_name, numsubexprs);
    if (numsubexprs > holding.size())
        throw std::runtime_error("ERROR: " + op_name + " cannot be parsed");
    for (size_t i = 1; i <= numsubexprs; ++i)
    {
        ASTNode *subexpr = holding.top();
        node->insert_subexpr_n(subexpr, numsubexprs - i);
        holding.pop();
    }
    holding.push(node);
}

void ASTDriver::createNode(std::string alias)
{
    ASTNode *node = new ASTNode("ALIAS", alias);
    holding.push(node);
}

void ASTDriver::createNode(std::string alias, indx_type indices)
{
    ASTNode *node = new ASTNode("INDEX", indices);
    node->pushback_subexpr(new ASTNode("ALIAS", alias));
    holding.push(node);
}

}
}
