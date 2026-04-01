/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VariableDerived.h"
#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace core
{

VariableDerived::VariableDerived(const std::string &name, adios2::derived::ExprNode exprTree,
                                 adios2::derived::ExprCodeStream codeStream,
                                 const std::string &exprString, const DataType exprType,
                                 const Dims &shape, const Dims &start, const Dims &count,
                                 const bool isConstant, const DerivedVarType varType,
                                 const std::map<std::string, DataType> nameToType)
: VariableBase(name, exprType, helper::GetDataTypeSize(exprType), shape, start, count, isConstant),
  m_DerivedType(varType), m_NameToType(nameToType), m_ExprTree(std::move(exprTree)),
  m_CodeStream(std::move(codeStream)), m_ExprString(exprString)
{
    if (varType != DerivedVarType::StoreData)
        m_WriteData = false;
}

DerivedVarType VariableDerived::GetDerivedType() { return m_DerivedType; }

std::vector<std::string> VariableDerived::VariableNameList() { return m_CodeStream.InputVarNames; }

void VariableDerived::UpdateExprDim(std::map<std::string, std::tuple<Dims, Dims, Dims>> NameToDims)
{
    auto outDims = adios2::derived::GetDims(m_CodeStream, NameToDims);
    m_Shape = std::get<2>(outDims);
    m_Start = std::get<0>(outDims);
    m_Count = std::get<1>(outDims);
    if (!m_Shape.empty())
        m_ShapeID = ShapeID::GlobalArray;
}

std::vector<std::tuple<void *, Dims, Dims>>
VariableDerived::ApplyExpression(std::map<std::string, std::unique_ptr<MinVarInfo>> &NameToMVI,
                                 bool DoCompute)
{
    size_t numBlocks = 0;
    // check that all variables have the same number of blocks
    for (const auto &variable : NameToMVI)
    {
        if (numBlocks == 0)
            numBlocks = variable.second->BlocksInfo.size();
        if (numBlocks != variable.second->BlocksInfo.size())
            helper::Throw<std::invalid_argument>("Core", "VariableDerived", "ApplyExpression",
                                                 " variables do not have the same number of blocks "
                                                 " in computing the derived variable " +
                                                     m_Name);
    }

    if (!DoCompute)
        return CreateEmptyData(NameToMVI, numBlocks);

    std::map<std::string, std::vector<adios2::derived::DerivedData>> inputData;
    // create the map between variable name and DerivedData object
    for (const auto &variable : NameToMVI)
    {
        // add the dimensions of all blocks into a vector
        std::vector<adios2::derived::DerivedData> varData;
        for (size_t i = 0; i < numBlocks; i++)
        {
            Dims start;
            Dims count;
            DataType type = m_NameToType[variable.first];
            if (m_ShapeID == ShapeID::JoinedArray)
                start = {};
            for (int d = 0; d < variable.second->Dims; d++)
            {
                if (m_ShapeID != ShapeID::JoinedArray)
                    start.push_back(variable.second->BlocksInfo[i].Start[d]);
                count.push_back(variable.second->BlocksInfo[i].Count[d]);
            }
            varData.push_back(adios2::derived::DerivedData(
                {variable.second->BlocksInfo[i].BufferP, start, count, type}));
        }
        inputData.insert({variable.first, varData});
    }
    std::vector<adios2::derived::DerivedData> outputData =
        adios2::derived::Execute(m_CodeStream, numBlocks, inputData);

    std::vector<std::tuple<void *, Dims, Dims>> blockData;
    for (size_t i = 0; i < numBlocks; i++)
    {
        blockData.push_back({outputData[i].Data, outputData[i].Start, outputData[i].Count});
    }

    return blockData;
}

std::vector<std::tuple<void *, Dims, Dims>>
VariableDerived::CreateEmptyData(std::map<std::string, std::unique_ptr<MinVarInfo>> &NameToVarInfo,
                                 size_t numBlocks)
{
    std::vector<std::tuple<void *, Dims, Dims>> blockData;
    for (size_t i = 0; i < numBlocks; i++)
    {
        std::map<std::string, std::tuple<Dims, Dims, Dims>> nameToDims;
        for (const auto &variable : NameToVarInfo)
        {
            Dims start;
            Dims count;
            if (m_ShapeID == ShapeID::JoinedArray)
                start = {};
            for (int d = 0; d < variable.second->Dims; d++)
            {
                if (m_ShapeID != ShapeID::JoinedArray)
                    start.push_back(variable.second->BlocksInfo[i].Start[d]);
                count.push_back(variable.second->BlocksInfo[i].Count[d]);
            }
            std::tuple<Dims, Dims, Dims> varDims({start, count, m_Shape});
            nameToDims.insert({variable.first, varDims});
        }

        auto outputDims = adios2::derived::GetDims(m_CodeStream, nameToDims);

        blockData.push_back({nullptr, std::get<0>(outputDims), std::get<1>(outputDims)});
    }
    return blockData;
}

} // end namespace core
} // end namespace adios2
