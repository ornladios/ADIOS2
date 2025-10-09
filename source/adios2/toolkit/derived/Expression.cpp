#ifndef ADIOS2_DERIVED_Expression_CPP_
#define ADIOS2_DERIVED_Expression_CPP_

#include "Expression.h"
#include "Function.h"
#include "adios2/helper/adiosLog.h"
#include "parser/ASTDriver.h"

#include <algorithm>
#include <functional>

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
    {ExpressionOperator::OP_SUBTRACT, {"SUBTRACT", true}},
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
    {"SUBTRACT", ExpressionOperator::OP_SUBTRACT}, {"DIV", ExpressionOperator::OP_DIV},
    {"DIVIDE", ExpressionOperator::OP_DIV},        {"MULT", ExpressionOperator::OP_MULT},
    {"MULTIPLY", ExpressionOperator::OP_MULT},     {"SQRT", ExpressionOperator::OP_SQRT},
    {"POW", ExpressionOperator::OP_POW},           {"SIN", ExpressionOperator::OP_SIN},
    {"COS", ExpressionOperator::OP_COS},           {"^", ExpressionOperator::OP_POW},
    {"TAN", ExpressionOperator::OP_TAN},           {"ASIN", ExpressionOperator::OP_ASIN},
    {"ACOS", ExpressionOperator::OP_ACOS},         {"ATAN", ExpressionOperator::OP_ATAN},
    {"MAGNITUDE", ExpressionOperator::OP_MAGN},    {"CROSS", ExpressionOperator::OP_CROSS},
    {"CURL", ExpressionOperator::OP_CURL}};

inline std::string get_op_name(ExpressionOperator op) { return op_property.at(op).name; }

inline bool get_op_associativity(ExpressionOperator op)
{
    return op_property.at(op).is_associative;
}

inline ExpressionOperator get_op(std::string op)
{
    std::transform(op.begin(), op.end(), op.begin(),
                   [](unsigned char c) { return std::toupper(c); });
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
        case adios2::detail::ExpressionOperator::OP_NUM: // add number
            exp.AddNumChild(e->get_number());
            break;
        default: // if the children nodes are other expressions, convert them to expressions
            adios2::derived::Expression temp_node("");
            ASTNode_to_ExpressionTree(e, temp_node);
            // move from a binary to a multinary tree if the child has the same operation
            if (convert_op(e->get_opname()) == convert_op(node->get_opname()) &&
                adios2::detail::get_op_associativity(convert_op(e->get_opname())))
            {
                auto subExpr = temp_node.GetChildren();
                // concatenate exprTree with temp_node
                for (auto childTree : subExpr)
                {
                    if (std::get<2>(childTree) == true)
                        exp.AddExpChild(std::get<0>(childTree));
                    else
                        exp.AddVarChild(std::get<1>(childTree));
                }
                // move the constants to the current node
                auto constants = temp_node.GetConstants();
                for (const auto &c : constants)
                    exp.AddNumChild(c);
            }
            else
            {
                exp.AddExpChild(temp_node);
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
    std::function<std::tuple<Dims, Dims, Dims>(std::vector<std::tuple<Dims, Dims, Dims>>, bool)>
        DimsFct;
    std::function<DataType(DataType)> TypeFct;
};

std::map<adios2::detail::ExpressionOperator, OperatorFunctions> OpFunctions = {
    {adios2::detail::ExpressionOperator::OP_ADD, {AddFunc, SameDimsWithAgrFunc, SameTypeFunc}},
    {adios2::detail::ExpressionOperator::OP_SUBTRACT, {SubtractFunc, SameDimsFunc, SameTypeFunc}},
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
        // if the sub_expression is a leaf, we get the type of the current oeration
        if (!std::get<2>(subexp))
        {
            DataType varType = NameToType[std::get<1>(subexp)];
            exprType.push_back(varType);
        }
        else
        {
            exprType.push_back(std::get<0>(subexp).GetType(NameToType));
        }
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
    std::vector<std::tuple<Dims, Dims, Dims>> exprDims;
    for (auto subexp : m_SubExprs)
    {
        // if the sub_expression is a leaf, we get the shape from the input std::map
        if (!std::get<2>(subexp))
        {
            exprDims.push_back(NameToDims[std::get<1>(subexp)]);
        }
        else
        {
            exprDims.push_back(std::get<0>(subexp).GetDims(NameToDims));
        }
    }
    // get the output dimensions after applying the operator
    auto op_fct = OpFunctions.at(m_Operator);
    auto opDims = op_fct.DimsFct(exprDims, m_Consts.size() > 0);
    return opDims;
}

std::vector<std::tuple<Expression, std::string, bool>> Expression::GetChildren()
{
    return m_SubExprs;
}

std::vector<std::string> Expression::GetConstants() { return m_Consts; }

void Expression::SetDims(std::map<std::string, std::tuple<Dims, Dims, Dims>> NameToDims)
{
    auto outDims = GetDims(NameToDims);
    m_Count = std::get<1>(outDims);
    m_Start = std::get<0>(outDims);
    m_Shape = std::get<2>(outDims);
}

void Expression::SetOperationType(adios2::detail::ExpressionOperator op) { m_Operator = op; }

std::vector<DerivedData>
Expression::ApplyExpression(const size_t numBlocks,
                            std::map<std::string, std::vector<DerivedData>> nameToData)
{
    // create operands for the computation function
    // exprData[0] = list of void* data for block 0 for each variable
    std::vector<std::vector<DerivedData>> exprData(numBlocks);
    std::vector<bool> deallocate;
    for (auto subexp : m_SubExprs)
    {
        // leafs
        if (!std::get<2>(subexp))
        {
            // do not deallocate leafs (this is user data)
            deallocate.push_back(false);
            // get the operands data for each block
            for (size_t blk = 0; blk < numBlocks; blk++)
            {
                exprData[blk].push_back(nameToData[std::get<1>(subexp)][blk]);
            }
        }
        else // there is a sub-expression
        {
            deallocate.push_back(true);
            // get the operands data for each block
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
    // get the type of the output data
    std::vector<DataType> exprType;
    for (auto op : exprData[0])
        exprType.push_back(op.Type);
    DataType outType = op_fct.TypeFct(exprType[0]);
    for (size_t blk = 0; blk < numBlocks; blk++)
    {
        // get the output dimension for each block
        std::vector<std::tuple<Dims, Dims, Dims>> exprDims;
        for (auto op : exprData[blk])
        {
            auto start = op.Start;
            auto count = op.Count;
            exprDims.push_back({start, count, count});
        }
        auto outDims = op_fct.DimsFct(exprDims, m_Consts.size() > 0);

        // add constants to the input data for applying the operator
        ExprData exprInputData({exprData[blk], m_Consts, outType});
        // apply function over the operands
        outputData[blk] = op_fct.ComputeFct(exprInputData);
        // set the dimension and type of the output data
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

void Expression::AddExpChild(Expression exp) { m_SubExprs.push_back({exp, "", true}); }

void Expression::AddVarChild(std::string var) { m_SubExprs.push_back({Expression(), var, false}); }

void Expression::AddNumChild(std::string c) { m_Consts.push_back(c); }

std::string Expression::toStringExpr()
{
    std::string result = "";
    result += get_op_name(m_Operator) + "(";
    for (std::tuple<Expression, std::string, bool> t : m_SubExprs)
    {
        if (std::get<2>(t) == true)
        {
            result += std::get<0>(t).toStringExpr();
        }
        else
        {
            result += "{" + std::get<1>(t) + "}";
        }
        result += ",";
    }
    // remove last comma
    result.pop_back();
    result += ")";

    return result;
}

std::vector<std::string> Expression::VariableNameList()
{
    std::vector<std::string> var_list;
    for (auto subexp : m_SubExprs)
    {
        // if the sub_expression is a leaf
        if (!std::get<2>(subexp))
        {
            var_list.push_back(std::get<1>(subexp));
        }
        else
        {
            auto subexpr_list = std::get<0>(subexp).VariableNameList();
            var_list.insert(var_list.end(), subexpr_list.begin(), subexpr_list.end());
        }
    }
    return var_list;
}

void Expression::Print()
{
    std::cout << "Print Expression:" << std::endl;
    std::cout << "\toperation: " << get_op_name(m_Operator) << std::endl;
    std::cout << "\tconstants: " << m_Consts.size() << std::endl;
    std::cout << "\t\tvalues: ";
    for (const auto &v : m_Consts)
        std::cout << v << " ";
    std::cout << std::endl;
    std::cout << "\tchildren: " << m_SubExprs.size() << std::endl;

    for (std::tuple<Expression, std::string, bool> t : m_SubExprs)
    {
        if (std::get<2>(t) == true)
        {
            std::get<0>(t).Print();
        }
        else
        {
            std::cout << "string: " << std::get<1>(t) << std::endl;
        }
    }
}

}
}
#endif
