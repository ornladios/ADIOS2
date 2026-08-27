/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_ExprNode_H_
#define ADIOS2_DERIVED_ExprNode_H_

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace adios2
{
// Forward-declare DataType so ExprNode can store it without pulling in ADIOSTypes.h
// (ExprNode.h is used by the parser library which doesn't have the full ADIOS includes)
enum class DataType;
}

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
    OP_ATTR,  /* Parser-use only */
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
    OP_CURL,
    OP_PROMOTE
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
    std::string VarName;  // set for variable leaves
    std::string Const;    // set for numeric constant leaves
    std::string AttrName; // set for attribute leaves (folded to Const at Define)
    DataType Type{};      // resolved by ResolveTreeTypes (default: DataType::None = 0)

    bool IsLeaf() const { return Children.empty(); }
    bool IsVar() const { return IsLeaf() && !VarName.empty(); }
    bool IsConst() const { return IsLeaf() && !Const.empty(); }
};

// Free functions operating on ExprNode trees
std::vector<std::string> VariableNameList(const ExprNode &node);
std::string ToStringExpr(const ExprNode &node);

/** Fold attribute leaves into numeric constants.  The resolver maps an
    attribute name to its numeric value; it should throw (with a helpful
    message) for unknown, non-numeric, or non-single-element attributes.
    Called by the IO layer at Define time, before VariableNameList. */
// Names of attributes referenced anywhere in the tree ("x = @name" bindings or
// inline "@name"). These appear in VariableNameList as "@name" pseudo-inputs;
// the IO layer injects each attribute's current value at every evaluation.
std::vector<std::string> AttributeNameList(const ExprNode &node);

}

namespace detail
{
// Parse an expression string into an ExprNode tree (uses flex/bison parser)
adios2::derived::ExprNode ParseToExprNode(const std::string &exprString);
}

}
#endif
