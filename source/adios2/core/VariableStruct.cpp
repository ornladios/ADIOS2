/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableStruct.cpp
 *
 *  Created on: Apr 24, 2022
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "VariableStruct.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace core
{

VariableStruct::VariableStruct(const std::string &name,
                               const size_t elementSize, const Dims &shape,
                               const Dims &start, const Dims &count,
                               const bool constantDims)
: VariableBase(name, DataType::Struct, elementSize, shape, start, count,
               constantDims)
{
}

void VariableStruct::AddSubVariable(const std::string &name,
                                    const DataType type)
{
    if (m_SizeAdded + helper::GetDataTypeSize(type) > m_ElementSize)
    {
        helper::Throw<std::invalid_argument>(
            "core", "VariableStruct", "AddSubVariable",
            "total sub-variable size " +
                std::to_string(m_SizeAdded + helper::GetDataTypeSize(type)) +
                " exceeded struct size " + std::to_string(m_ElementSize) +
                " for struct variable " + m_Name);
    }
    m_VarList.push_back({name, type});
    m_SizeAdded += helper::GetDataTypeSize(type);
}

const std::vector<std::pair<std::string, DataType>> &
VariableStruct::SubVariableList()
{
    return m_VarList;
}

} // end namespace core
} // end namespace adios2
