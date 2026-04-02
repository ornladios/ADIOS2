/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_CORE_VARIABLE_DERIVED_H_
#define ADIOS2_CORE_VARIABLE_DERIVED_H_

#include "adios2/core/VariableBase.h"
#include "adios2/toolkit/derived/ExprCodeStream.h"
#include "adios2/toolkit/derived/ExprNode.h"

namespace adios2
{
namespace core
{

/**
 * @param Base (parent) class for template derived (child) class Variable.
 */
class VariableDerived : public VariableBase
{
    DerivedVarType m_DerivedType;
    std::map<std::string, DataType> m_NameToType;

    std::vector<std::tuple<void *, Dims, Dims>>
    CreateEmptyData(std::map<std::string, std::unique_ptr<MinVarInfo>> &NameToVarInfo,
                    size_t numBlocks);

public:
    adios2::derived::ExprNode m_ExprTree;
    adios2::derived::ExprCodeStream m_CodeStream;
    std::string m_ExprString;

    VariableDerived(const std::string &name, adios2::derived::ExprNode exprTree,
                    adios2::derived::ExprCodeStream codeStream, const std::string &exprString,
                    const DataType exprType, const Dims &shape, const Dims &start,
                    const Dims &count, const bool isConstant, const DerivedVarType varType,
                    const std::map<std::string, DataType> nameToType);
    ~VariableDerived() = default;

    DerivedVarType GetDerivedType();
    std::vector<std::string> VariableNameList();
    void UpdateExprDim(std::map<std::string, std::tuple<Dims, Dims, Dims>> NameToDims);

    std::vector<std::tuple<void *, Dims, Dims>>
    ApplyExpression(std::map<std::string, std::unique_ptr<MinVarInfo>> &mvi, bool DoCompute = true);
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_DERIVED_H_ */
