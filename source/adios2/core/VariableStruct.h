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

    struct BPInfo
    {
        Dims Shape;
        Dims Start;
        Dims Count;
        Dims MemoryStart;
        Dims MemoryCount;
        std::vector<std::shared_ptr<Operator>> Operations;
        size_t Step = 0;
        size_t StepsStart = 0;
        size_t StepsCount = 0;
        size_t BlockID = 0;
        void *Data = nullptr;
        void *BufferP = nullptr;
        int WriterID = 0;
        SelectionType Selection = SelectionType::BoundingBox;
        bool IsValue = false;
        bool IsReverseDims = false;
        bool IsGPU = false;
    };

    std::vector<BPInfo> m_BlocksInfo;

    struct VarList
    {
        std::string Name;
        DataType Type;
        size_t Size;
    };

    std::vector<VarList> m_TypeDefinition;

    VariableStruct(const std::string &name, const size_t elementSize,
                   const Dims &shape, const Dims &start, const Dims &count,
                   const bool constantDims);

    ~VariableStruct() = default;

    void AddSubVariable(const std::string &name, const DataType type,
                        const size_t size);

    void SetData(const void *data) noexcept;

    void *GetData() const noexcept;

private:
    size_t m_SizeAdded = 0;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLESTRUCT_H_ */
