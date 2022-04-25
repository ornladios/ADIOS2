/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableStruct.h
 *
 *  Created on: Apr 24, 2022
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#ifndef ADIOS2_CORE_VARIABLESTRUCT_H_
#define ADIOS2_CORE_VARIABLESTRUCT_H_

#include "VariableBase.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace core
{

class VariableStruct : public VariableBase
{

public:
    void *m_Data = nullptr;

    VariableStruct(const std::string &name, const size_t elementSize,
                   const Dims &shape, const Dims &start, const Dims &count,
                   const bool constantDims);

    ~VariableStruct() = default;

    void AddSubVariable(const std::string &name, const DataType type);

    const std::vector<std::pair<std::string, DataType>> &SubVariableList();

    void SetData(const void *data) noexcept;

    void *GetData() const noexcept;

private:
    std::vector<std::pair<std::string, DataType>> m_VarList;
    size_t m_SizeAdded = 0;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLESTRUCT_H_ */
