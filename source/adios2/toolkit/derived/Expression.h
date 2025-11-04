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
/*
 A Note on Expression:
 - Sub expressions can include another operation nodes or variable names
    - the third entry in the tuple distinguishes between variable and operation
 - The type of the operation
 - Constants are used to compute the operation [e.g. log_2, var + 2, etc.]
 */
class Expression
{
    adios2::detail::ExpressionOperator m_Operator;
    // the subexpressions can be other expressions or variables
    std::vector<std::tuple<Expression, std::string, bool>> m_SubExprs;
    // a set of constants attached to the operation
    std::vector<std::string> m_Consts;

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
    std::vector<std::tuple<Expression, std::string, bool>> GetChildren();
    std::vector<std::string> GetConstants();

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
