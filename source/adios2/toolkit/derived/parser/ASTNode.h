/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASTNODE_HH
#define ASTNODE_HH
#include <string>
#include <tuple>
#include <vector>

namespace adios2
{
namespace detail
{

class ASTNode
{
public:
    ASTNode();
    ASTNode(std::string);
    ASTNode(std::string, size_t);
    ASTNode(std::string, std::string);
    ASTNode(std::string, double);
    ASTNode(std::string, std::vector<std::tuple<int, int, int>>);
    ~ASTNode();

    void set_num_subexprs(size_t);
    void pushback_subexpr(ASTNode *);
    void insert_subexpr_n(ASTNode *, size_t);
    std::string printpretty(std::string = "");

    std::vector<ASTNode *> get_subexprs();
    std::string get_opname();
    std::string get_alias();
    std::string get_varname();
    std::vector<std::tuple<int, int, int>> get_indices();
    std::string get_number();

    void set_varname(const std::string);
    void set_indices(const std::vector<std::tuple<int, int, int>>);
    /* rewrite this node into a numeric constant leaf (named constants) */
    void to_number(const std::string &value_str);
    /* rewrite this node into an attribute leaf (value folded at Define) */
    void to_attribute(const std::string &attr_name);

private:
    std::vector<ASTNode *> sub_exprs;
    std::string opname;
    std::string alias;
    std::string varname;
    std::vector<std::tuple<int, int, int>> indices;
    std::string value;
};

}
}
#endif // ! ASTNODE_HH
