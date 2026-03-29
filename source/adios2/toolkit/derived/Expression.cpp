/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_DERIVED_Expression_CPP_
#define ADIOS2_DERIVED_Expression_CPP_

#include "Expression.h"
#include "Function.h"
#include "adios2/helper/adiosLog.h"
#include "parser/ASTDriver.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <sstream>

namespace adios2
{
namespace detail
{
struct OperatorProperty
{
    std::string name;
    bool is_associative;
};

const std::map<ExpressionOperator, OperatorProperty> op_property = {
    {ExpressionOperator::OP_NULL, {"NULL", false}},
    {ExpressionOperator::OP_ALIAS, {"ALIAS", false}}, /* Parser-use only */
    {ExpressionOperator::OP_PATH, {"PATH", false}},   /* Parser-use only */
    {ExpressionOperator::OP_NUM, {"NUM", false}},     /* Parser-use only */
    {ExpressionOperator::OP_INDEX, {"INDEX", false}},
    {ExpressionOperator::OP_ADD, {"ADD", true}},
    {ExpressionOperator::OP_SUBTRACT, {"SUBTRACT", false}},
    {ExpressionOperator::OP_NEGATE, {"NEGATE", false}},
    {ExpressionOperator::OP_MULT, {"MULT", true}},
    {ExpressionOperator::OP_DIV, {"DIV", false}},
    {ExpressionOperator::OP_SQRT, {"SQRT", false}},
    {ExpressionOperator::OP_POW, {"POW", false}},
    {ExpressionOperator::OP_SIN, {"SIN", false}},
    {ExpressionOperator::OP_COS, {"COS", false}},
    {ExpressionOperator::OP_TAN, {"TAN", false}},
    {ExpressionOperator::OP_ASIN, {"ASIN", false}},
    {ExpressionOperator::OP_ACOS, {"ACOS", false}},
    {ExpressionOperator::OP_ATAN, {"ATAN", false}},
    {ExpressionOperator::OP_MAGN, {"MAGNITUDE", false}},
    {ExpressionOperator::OP_CROSS, {"CROSS", false}},
    {ExpressionOperator::OP_CURL, {"CURL", false}}};

const std::map<std::string, ExpressionOperator> string_to_op = {
    {"ALIAS", ExpressionOperator::OP_ALIAS}, /* Parser-use only */
    {"PATH", ExpressionOperator::OP_PATH},   /* Parser-use only */
    {"NUM", ExpressionOperator::OP_NUM},     /* Parser-use only */
    {"INDEX", ExpressionOperator::OP_INDEX},       {"SUM", ExpressionOperator::OP_ADD},
    {"ADD", ExpressionOperator::OP_ADD},           {"MINUS", ExpressionOperator::OP_SUBTRACT},
    {"SUBTRACT", ExpressionOperator::OP_SUBTRACT}, {"NEGATE", ExpressionOperator::OP_NEGATE},
    {"DIV", ExpressionOperator::OP_DIV},           {"DIVIDE", ExpressionOperator::OP_DIV},
    {"MULT", ExpressionOperator::OP_MULT},         {"MULTIPLY", ExpressionOperator::OP_MULT},
    {"SQRT", ExpressionOperator::OP_SQRT},         {"POW", ExpressionOperator::OP_POW},
    {"SIN", ExpressionOperator::OP_SIN},           {"^", ExpressionOperator::OP_POW},
    {"COS", ExpressionOperator::OP_COS},           {"TAN", ExpressionOperator::OP_TAN},
    {"ASIN", ExpressionOperator::OP_ASIN},         {"ACOS", ExpressionOperator::OP_ACOS},
    {"ATAN", ExpressionOperator::OP_ATAN},         {"MAGNITUDE", ExpressionOperator::OP_MAGN},
    {"CROSS", ExpressionOperator::OP_CROSS},       {"CURL", ExpressionOperator::OP_CURL}};

inline std::string get_op_name(ExpressionOperator op) { return op_property.at(op).name; }

inline bool get_op_associativity(ExpressionOperator op)
{
    return op_property.at(op).is_associative;
}

inline ExpressionOperator get_op(std::string op)
{
    std::transform(op.begin(), op.end(), op.begin(), [](auto c) { return std::toupper(c); });
    return string_to_op.at(op);
}

// helper function
ExpressionOperator convert_op(std::string opname)
{
    adios2::detail::ExpressionOperator op;
    try
    {
        op = adios2::detail::get_op(opname);
    }
    catch (std::out_of_range &e)
    {
        (void)e; // use e
        helper::Throw<std::invalid_argument>("Derived", "ExprHelper", "get_op",
                                             "Parser cannot recognize operator '" + opname + "'.");
    }
    return op;
};

void ASTNode_to_ExpressionTree(adios2::detail::ASTNode *node, adios2::derived::Expression &exp)
{
    exp.SetOperationType(convert_op(node->get_opname()));
    for (adios2::detail::ASTNode *e : node->get_subexprs())
    {
        switch (convert_op(e->get_opname()))
        {
        case adios2::detail::ExpressionOperator::OP_ALIAS: // add variable given by alias
            exp.AddVarChild(e->get_varname());
            break;
        case adios2::detail::ExpressionOperator::OP_PATH: // add variable name
            exp.AddVarChild(e->get_varname());
            break;
        case adios2::detail::ExpressionOperator::OP_NUM: // add constant as positional child
            exp.AddNumChild(e->get_number());
            break;
        default: // if the children nodes are other expressions, convert them to expressions
        {
            // Constant folding: NEGATE applied to a single constant becomes a negated constant
            if (convert_op(e->get_opname()) == adios2::detail::ExpressionOperator::OP_NEGATE)
            {
                auto subexprs = e->get_subexprs();
                if (subexprs.size() == 1 && convert_op(subexprs[0]->get_opname()) ==
                                                adios2::detail::ExpressionOperator::OP_NUM)
                {
                    std::string negVal = "-" + subexprs[0]->get_number();
                    exp.AddNumChild(negVal);
                    break;
                }
            }
            adios2::derived::Expression temp_node("");
            ASTNode_to_ExpressionTree(e, temp_node);
            // move from a binary to a multinary tree if the child has the same operation
            if (convert_op(e->get_opname()) == convert_op(node->get_opname()) &&
                adios2::detail::get_op_associativity(convert_op(e->get_opname())))
            {
                for (auto childTree : temp_node.GetChildren())
                {
                    auto childType = std::get<2>(childTree);
                    if (childType == adios2::derived::ChildType::EXPR_CHILD)
                        exp.AddExpChild(std::get<0>(childTree));
                    else if (childType == adios2::derived::ChildType::VAR_CHILD)
                        exp.AddVarChild(std::get<1>(childTree));
                    else // CONST_CHILD
                        exp.AddNumChild(std::get<1>(childTree));
                }
            }
            else
            {
                exp.AddExpChild(temp_node);
            }
        }
        }
    }
}

}

namespace derived
{
struct OperatorFunctions
{
    std::function<DerivedData(ExprData)> ComputeFct;
    std::function<std::tuple<Dims, Dims, Dims>(std::vector<std::tuple<Dims, Dims, Dims>>)> DimsFct;
    std::function<DataType(DataType)> TypeFct;
};

std::map<adios2::detail::ExpressionOperator, OperatorFunctions> OpFunctions = {
    {adios2::detail::ExpressionOperator::OP_ADD, {AddFunc, SameDimsWithAgrFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_SUBTRACT, {SubtractFunc, SameDimsFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_NEGATE, {NegateFunc, SameDimsFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_MULT, {MultFunc, SameDimsFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_DIV, {DivFunc, SameDimsFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_POW, {PowFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_SQRT, {SqrtFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_SIN, {SinFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_COS, {CosFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_TAN, {TanFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_ASIN, {AsinFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_ACOS, {AcosFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_ATAN, {AtanFunc, SameDimsFunc, FloatTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_MAGN,
     {MagnitudeFunc, SameDimsWithAgrFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_CROSS, {Cross3DFunc, Cross3DDimsFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_CURL, {Curl3DFunc, CurlDimsFunc, SameTypeFunc}}};

Expression::Expression(std::string string_exp)
: m_Operator(adios2::detail::ExpressionOperator::OP_NULL), m_Shape({0}), m_Start({0}), m_Count({0}),
  m_ExprString(string_exp)
{
    if (string_exp != "")
    {
        adios2::detail::ASTDriver drv(string_exp);
        adios2::detail::ASTNode_to_ExpressionTree(drv.getAST(), *this);
    }
}

Dims Expression::GetShape() { return m_Shape; }

Dims Expression::GetStart() { return m_Start; }

Dims Expression::GetCount() { return m_Count; }

DataType Expression::GetType(std::map<std::string, DataType> NameToType)
{
    std::vector<DataType> exprType;
    for (auto subexp : m_SubExprs)
    {
        auto childType = std::get<2>(subexp);
        if (childType == ChildType::VAR_CHILD)
        {
            DataType varType = NameToType[std::get<1>(subexp)];
            exprType.push_back(varType);
        }
        else if (childType == ChildType::EXPR_CHILD)
        {
            exprType.push_back(std::get<0>(subexp).GetType(NameToType));
        }
        // CONST_CHILD: skip, constants don't determine type
    }

    // check that all types are the same
    for (size_t i = 1; i < exprType.size(); i++)
        if (exprType[i - 1] != exprType[i])
            helper::Throw<std::invalid_argument>(
                "Derived", "Expression", "GetType",
                "Derived expression operators are not the same type");
    // get the output type after applying the operator
    auto op_fct = OpFunctions.at(m_Operator);
    DataType opType = op_fct.TypeFct(exprType[0]);
    return opType;
}

std::tuple<Dims, Dims, Dims>
Expression::GetDims(std::map<std::string, std::tuple<Dims, Dims, Dims>> NameToDims)
{
    // First pass: find reference dims from first non-constant child
    std::tuple<Dims, Dims, Dims> refDims = {{}, {}, {}};
    for (auto subexp : m_SubExprs)
    {
        auto childType = std::get<2>(subexp);
        if (childType == ChildType::VAR_CHILD)
        {
            refDims = NameToDims[std::get<1>(subexp)];
            break;
        }
        else if (childType == ChildType::EXPR_CHILD)
        {
            refDims = std::get<0>(subexp).GetDims(NameToDims);
            break;
        }
    }

    // Second pass: build dims list, using refDims for constants
    std::vector<std::tuple<Dims, Dims, Dims>> exprDims;
    for (auto subexp : m_SubExprs)
    {
        auto childType = std::get<2>(subexp);
        if (childType == ChildType::VAR_CHILD)
        {
            exprDims.push_back(NameToDims[std::get<1>(subexp)]);
        }
        else if (childType == ChildType::EXPR_CHILD)
        {
            exprDims.push_back(std::get<0>(subexp).GetDims(NameToDims));
        }
        else // CONST_CHILD: broadcast to match sibling dims
        {
            exprDims.push_back(refDims);
        }
    }
    // get the output dimensions after applying the operator
    auto op_fct = OpFunctions.at(m_Operator);
    auto opDims = op_fct.DimsFct(exprDims);
    return opDims;
}

std::vector<std::tuple<Expression, std::string, ChildType>> Expression::GetChildren()
{
    return m_SubExprs;
}

void Expression::SetDims(std::map<std::string, std::tuple<Dims, Dims, Dims>> NameToDims)
{
    auto outDims = GetDims(NameToDims);
    m_Count = std::get<1>(outDims);
    m_Start = std::get<0>(outDims);
    m_Shape = std::get<2>(outDims);
}

void Expression::SetOperationType(adios2::detail::ExpressionOperator op) { m_Operator = op; }

// Helper: create a DerivedData filled with a constant value, broadcast to dataSize elements
template <class T>
static DerivedData MakeConstantData(const std::string &valueStr, size_t dataSize, Dims start,
                                    Dims count, DataType type)
{
    T *buf = (T *)malloc(dataSize * sizeof(T));
    std::istringstream iss(valueStr);
    T val;
    iss >> val;
    for (size_t i = 0; i < dataSize; i++)
        buf[i] = val;
    return DerivedData({(void *)buf, start, count, type});
}

static DerivedData BroadcastConstant(const std::string &valueStr, size_t dataSize, Dims start,
                                     Dims count, DataType type)
{
#define declare_type_broadcast(T)                                                                  \
    if (type == helper::GetDataType<T>())                                                          \
        return MakeConstantData<T>(valueStr, dataSize, start, count, type);
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type_broadcast)
    helper::Throw<std::invalid_argument>("Derived", "Expression", "BroadcastConstant",
                                         "Invalid variable types");
    return DerivedData();
}

std::vector<DerivedData>
Expression::ApplyExpression(const size_t numBlocks,
                            std::map<std::string, std::vector<DerivedData>> nameToData)
{
    // create operands for the computation function
    std::vector<std::vector<DerivedData>> exprData(numBlocks);
    std::vector<bool> deallocate;

    // Track which children are constants (and their values) for deferred broadcast
    struct ChildInfo
    {
        ChildType type;
        std::string constValue; // only for CONST_CHILD
    };
    std::vector<ChildInfo> childInfos;

    for (auto subexp : m_SubExprs)
    {
        auto childType = std::get<2>(subexp);
        childInfos.push_back({childType, std::get<1>(subexp)});

        if (childType == ChildType::VAR_CHILD)
        {
            deallocate.push_back(false);
            for (size_t blk = 0; blk < numBlocks; blk++)
            {
                exprData[blk].push_back(nameToData[std::get<1>(subexp)][blk]);
            }
        }
        else if (childType == ChildType::CONST_CHILD)
        {
            // Placeholder — will be filled after we know the shape from siblings
            deallocate.push_back(true);
            for (size_t blk = 0; blk < numBlocks; blk++)
            {
                exprData[blk].push_back(DerivedData());
            }
        }
        else // EXPR_CHILD
        {
            deallocate.push_back(true);
            auto subexpData = std::get<0>(subexp).ApplyExpression(numBlocks, nameToData);
            for (size_t blk = 0; blk < numBlocks; blk++)
            {
                exprData[blk].push_back(subexpData[blk]);
            }
        }
    }

    // apply the computation operator on all blocks
    std::vector<DerivedData> outputData(numBlocks);
    auto op_fct = OpFunctions.at(m_Operator);
    // get the type of the output data from the first non-constant operand
    DataType outType = DataType::None;
    for (size_t blk = 0; blk < numBlocks && outType == DataType::None; blk++)
    {
        for (size_t i = 0; i < childInfos.size(); i++)
        {
            if (childInfos[i].type != ChildType::CONST_CHILD)
            {
                outType = exprData[blk][i].Type;
                break;
            }
        }
    }
    if (outType != DataType::None)
        outType = op_fct.TypeFct(outType);
    else
        outType = DataType::Double; // all-constant expression defaults to double

    for (size_t blk = 0; blk < numBlocks; blk++)
    {
        // Determine shape from first non-constant operand in this block
        size_t dataSize = 0;
        Dims refStart, refCount;
        for (size_t i = 0; i < childInfos.size(); i++)
        {
            if (childInfos[i].type != ChildType::CONST_CHILD)
            {
                refStart = exprData[blk][i].Start;
                refCount = exprData[blk][i].Count;
                dataSize = std::accumulate(refCount.begin(), refCount.end(), (size_t)1,
                                           std::multiplies<size_t>());
                break;
            }
        }
        if (dataSize == 0)
            dataSize = 1; // all-constant expression

        // Now fill in constant children with broadcast data
        DataType constType = outType;
        // For type functions that promote (e.g. FloatTypeFunc), constants should match input type
        // Use the first non-constant operand's type for constants
        for (size_t i = 0; i < childInfos.size(); i++)
        {
            if (childInfos[i].type != ChildType::CONST_CHILD &&
                exprData[blk][i].Type != DataType::None)
            {
                constType = exprData[blk][i].Type;
                break;
            }
        }

        for (size_t i = 0; i < childInfos.size(); i++)
        {
            if (childInfos[i].type == ChildType::CONST_CHILD)
            {
                exprData[blk][i] = BroadcastConstant(childInfos[i].constValue, dataSize, refStart,
                                                     refCount, constType);
            }
        }

        // get dims for this block
        std::vector<std::tuple<Dims, Dims, Dims>> exprDims;
        for (auto &op : exprData[blk])
        {
            exprDims.push_back({op.Start, op.Count, op.Count});
        }
        auto outDims = op_fct.DimsFct(exprDims);

        // apply function over the operands
        ExprData exprInputData({exprData[blk], outType});
        outputData[blk] = op_fct.ComputeFct(exprInputData);
        outputData[blk].Type = outType;
        outputData[blk].Start = std::get<0>(outDims);
        outputData[blk].Count = std::get<1>(outDims);
    }
    // deallocate intermediate data after computing the operation
    for (size_t blk = 0; blk < numBlocks; blk++)
    {
        for (size_t i = 0; i < exprData[blk].size(); i++)
        {
            if (deallocate[i] == false)
                continue;
            free(exprData[blk][i].Data);
        }
    }
    return outputData;
}

void Expression::AddExpChild(Expression exp)
{
    m_SubExprs.push_back({exp, "", ChildType::EXPR_CHILD});
}

void Expression::AddVarChild(std::string var)
{
    m_SubExprs.push_back({Expression(), var, ChildType::VAR_CHILD});
}

void Expression::AddNumChild(std::string c)
{
    m_SubExprs.push_back({Expression(), c, ChildType::CONST_CHILD});
}

std::string Expression::toStringExpr()
{
    std::string result = "";
    result += get_op_name(m_Operator) + "(";
    for (auto &t : m_SubExprs)
    {
        auto childType = std::get<2>(t);
        if (childType == ChildType::EXPR_CHILD)
        {
            result += std::get<0>(t).toStringExpr();
        }
        else if (childType == ChildType::VAR_CHILD)
        {
            result += "{" + std::get<1>(t) + "}";
        }
        else // CONST_CHILD
        {
            result += std::get<1>(t);
        }
        result += ",";
    }
    // remove last comma
    if (!m_SubExprs.empty())
        result.pop_back();
    result += ")";

    return result;
}

std::vector<std::string> Expression::VariableNameList()
{
    std::vector<std::string> var_list;
    for (auto subexp : m_SubExprs)
    {
        auto childType = std::get<2>(subexp);
        if (childType == ChildType::VAR_CHILD)
        {
            var_list.push_back(std::get<1>(subexp));
        }
        else if (childType == ChildType::EXPR_CHILD)
        {
            auto subexpr_list = std::get<0>(subexp).VariableNameList();
            var_list.insert(var_list.end(), subexpr_list.begin(), subexpr_list.end());
        }
        // CONST_CHILD: skip
    }
    return var_list;
}

void Expression::Print()
{
    std::cout << "Print Expression:" << std::endl;
    std::cout << "\toperation: " << get_op_name(m_Operator) << std::endl;
    std::cout << "\tchildren: " << m_SubExprs.size() << std::endl;

    for (auto &t : m_SubExprs)
    {
        auto childType = std::get<2>(t);
        if (childType == ChildType::EXPR_CHILD)
        {
            std::get<0>(t).Print();
        }
        else if (childType == ChildType::VAR_CHILD)
        {
            std::cout << "variable: " << std::get<1>(t) << std::endl;
        }
        else // CONST_CHILD
        {
            std::cout << "constant: " << std::get<1>(t) << std::endl;
        }
    }
}

}
}
#endif
