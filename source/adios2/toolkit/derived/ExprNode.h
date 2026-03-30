/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_ExprNode_H_
#define ADIOS2_DERIVED_ExprNode_H_

#include <string>
#include <vector>

namespace adios2
{
namespace detail
{

enum ExpressionOperator
{
    OP_NULL,
    OP_ALIAS, /* Parser-use only */
    OP_PATH,  /* Parser-use only */
    OP_NUM,   /* Parser-use only */
    OP_INDEX,
    OP_ADD,
    OP_SUBTRACT,
    OP_NEGATE,
    OP_MULT,
    OP_DIV,
    OP_SQRT,
    OP_POW,
    OP_SIN,
    OP_COS,
    OP_TAN,
    OP_ASIN,
    OP_ACOS,
    OP_ATAN,
    OP_MAGN,
    OP_CROSS,
    OP_CURL
};

}

namespace derived
{

/**
 * Pure data struct representing a parsed expression tree.
 * This is the contract between the parser and everything downstream.
 * OP_NULL leaves are either variables (VarName set) or constants (Const set).
 */
struct ExprNode
{
    detail::ExpressionOperator Op = detail::ExpressionOperator::OP_NULL;
    std::vector<ExprNode> Children;
    std::string VarName; // set for variable leaves
    std::string Const;   // set for numeric constant leaves

    bool IsLeaf() const { return Children.empty(); }
    bool IsVar() const { return IsLeaf() && !VarName.empty(); }
    bool IsConst() const { return IsLeaf() && !Const.empty(); }
};

// Free functions operating on ExprNode trees
std::vector<std::string> VariableNameList(const ExprNode &node);
std::string ToStringExpr(const ExprNode &node);

}

namespace detail
{
// Parse an expression string into an ExprNode tree (uses flex/bison parser)
adios2::derived::ExprNode ParseToExprNode(const std::string &exprString);
}

}
#endif
