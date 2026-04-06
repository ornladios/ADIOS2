/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ASTDriver.h"
#include "adios2/toolkit/derived/ExprNode.h"

#include <algorithm>
#include <stdexcept>

namespace adios2
{
namespace detail
{

static const std::map<std::string, ExpressionOperator> string_to_op_new = {
    {"ALIAS", ExpressionOperator::OP_ALIAS},    {"PATH", ExpressionOperator::OP_PATH},
    {"NUM", ExpressionOperator::OP_NUM},        {"INDEX", ExpressionOperator::OP_INDEX},
    {"SUM", ExpressionOperator::OP_ADD},        {"ADD", ExpressionOperator::OP_ADD},
    {"MINUS", ExpressionOperator::OP_SUBTRACT}, {"SUBTRACT", ExpressionOperator::OP_SUBTRACT},
    {"NEGATE", ExpressionOperator::OP_NEGATE},  {"DIV", ExpressionOperator::OP_DIV},
    {"DIVIDE", ExpressionOperator::OP_DIV},     {"MULT", ExpressionOperator::OP_MULT},
    {"MULTIPLY", ExpressionOperator::OP_MULT},  {"SQRT", ExpressionOperator::OP_SQRT},
    {"POW", ExpressionOperator::OP_POW},        {"^", ExpressionOperator::OP_POW},
    {"SIN", ExpressionOperator::OP_SIN},        {"COS", ExpressionOperator::OP_COS},
    {"TAN", ExpressionOperator::OP_TAN},        {"ASIN", ExpressionOperator::OP_ASIN},
    {"ACOS", ExpressionOperator::OP_ACOS},      {"ATAN", ExpressionOperator::OP_ATAN},
    {"MAGNITUDE", ExpressionOperator::OP_MAGN}, {"CROSS", ExpressionOperator::OP_CROSS},
    {"CURL", ExpressionOperator::OP_CURL}};

static ExpressionOperator ConvertOp(const std::string &opname)
{
    std::string upper = opname;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](auto c) { return std::toupper(c); });
    auto it = string_to_op_new.find(upper);
    if (it == string_to_op_new.end())
    {
        throw std::invalid_argument("Unrecognized operator '" + opname + "' in derived expression");
    }
    return it->second;
}

static adios2::derived::ExprNode ASTNodeToExprNode(ASTNode *node)
{
    adios2::derived::ExprNode result;
    ExpressionOperator op = ConvertOp(node->get_opname());

    switch (op)
    {
    case ExpressionOperator::OP_ALIAS:
    case ExpressionOperator::OP_PATH:
        // Variable leaf
        result.Op = ExpressionOperator::OP_NULL;
        result.VarName = node->get_varname();
        return result;

    case ExpressionOperator::OP_NUM:
        // Constant leaf
        result.Op = ExpressionOperator::OP_NULL;
        result.Const = node->get_number();
        return result;

    default:
        break;
    }

    // Constant folding: NEGATE applied to a single constant → negated constant
    if (op == ExpressionOperator::OP_NEGATE)
    {
        auto subexprs = node->get_subexprs();
        if (subexprs.size() == 1 &&
            ConvertOp(subexprs[0]->get_opname()) == ExpressionOperator::OP_NUM)
        {
            result.Op = ExpressionOperator::OP_NULL;
            result.Const = "-" + subexprs[0]->get_number();
            return result;
        }
    }

    // Operator node: recursively convert children
    result.Op = op;
    for (ASTNode *child : node->get_subexprs())
    {
        result.Children.push_back(ASTNodeToExprNode(child));
    }
    return result;
}

adios2::derived::ExprNode ParseToExprNode(const std::string &exprString)
{
    ASTDriver drv(exprString);
    ASTNode *ast = drv.getAST();
    return ASTNodeToExprNode(ast);
}

}
}
