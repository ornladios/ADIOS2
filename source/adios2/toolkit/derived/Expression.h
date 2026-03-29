/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_Expression_H_
#define ADIOS2_DERIVED_Expression_H_

#include "DerivedData.h"
#include <string>
#include <unordered_map>

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

enum class ChildType
{
    EXPR_CHILD,
    VAR_CHILD,
    CONST_CHILD
};

/*
 A Note on Expression:
 - Sub expressions can include operation nodes, variable names, or constants
    - ChildType distinguishes between expression, variable, and constant children
 - The type of the operation
 */
class Expression
{
    adios2::detail::ExpressionOperator m_Operator;
    // children: (sub-expression, name-or-value string, child type)
    std::vector<std::tuple<Expression, std::string, ChildType>> m_SubExprs;

    Dims m_Shape;
    Dims m_Start;
    Dims m_Count;

    void Print();

public:
    std::string m_ExprString;

    Expression() = default;
    Expression(std::string expression);

    Dims GetShape();
    Dims GetStart();
    Dims GetCount();
    DataType GetType(std::map<std::string, DataType> NameToType);
    std::tuple<Dims, Dims, Dims>
    GetDims(std::map<std::string, std::tuple<Dims, Dims, Dims>> NameToDims);
    std::vector<std::tuple<Expression, std::string, ChildType>> GetChildren();

    void SetDims(std::map<std::string, std::tuple<Dims, Dims, Dims>> NameToDims);
    void SetOperationType(adios2::detail::ExpressionOperator op);

    std::vector<DerivedData>
    ApplyExpression(const size_t numBlocks,
                    std::map<std::string, std::vector<DerivedData>> nameToData);

    void AddExpChild(Expression exp);
    void AddVarChild(std::string var);
    void AddNumChild(std::string c);

    std::string toStringExpr();
    std::vector<std::string> VariableNameList();
};

}
}
#endif
