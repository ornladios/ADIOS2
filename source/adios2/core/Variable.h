/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.h : template class for self-describing primitive variables
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy
 */

#ifndef ADIOS2_CORE_VARIABLE_H_
#define ADIOS2_CORE_VARIABLE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <ostream> //std::ostream in MonitorGroups
#include <string>
#include <unordered_map>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/VariableBase.h"
#include "adios2/helper/adiosMath.h"
#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace core
{

template <class T>
class Span
{
public:
    std::pair<size_t, size_t> m_MinMaxDataPositions;
    std::pair<size_t, size_t> m_MinMaxMetadataPositions;
    size_t m_PayloadPosition = 0;
    T m_Value = T{};

    Span(Engine &engine, const size_t size);
    ~Span() = default;

    size_t Size() const noexcept;
    T *Data() const noexcept;

    T &At(const size_t position);
    const T &At(const size_t position) const;

    T &operator[](const size_t position);
    const T &operator[](const size_t position) const;

private:
    Engine &m_Engine;
    size_t m_Size = 0;
};

/**
 * @param Base (parent) class for template derived (child) class Variable.
 */
template <class T>
class Variable : public VariableBase
{

public:
    /** current reference to data */
    T *m_Data = nullptr;
    /** absolute minimum */
    T m_Min = T();
    /** absolute maximum */
    T m_Max = T();
    /** current value */
    T m_Value = T();

    struct Info
    {
        /** Contains (seek) read information for available [step][blockID],
         *  used in Read mode only,
         *  <pre>
         *  key: step
         *  value: blockID is the vector (map value) index
         *  </pre>
         */
        std::map<size_t, std::vector<helper::SubStreamBoxInfo>>
            StepBlockSubStreamsInfo;

        Dims Shape;
        Dims Start;
        Dims Count;
        Dims MemoryStart;
        Dims MemoryCount;
        std::vector<Operation> Operations;
        size_t Step = 0;
        size_t StepsStart = 0;
        size_t StepsCount = 0;
        size_t BlockID = 0;
        T *Data = nullptr;
        T Min = T();
        T Max = T();
        T Value = T();
        std::vector<T> MinMaxs; // sub-block level min-max
        struct helper::BlockDivisionInfo SubBlockInfo;
        T *BufferP = nullptr;
        std::vector<T> BufferV;
        int WriterID = 0;
        SelectionType Selection = SelectionType::BoundingBox;
        bool IsValue = false;
        bool IsReverseDims = false;
    };

    /** use for multiblock info */
    std::vector<Info> m_BlocksInfo;

    using Span = core::Span<T>;

    /** Needs a map to preserve iterator as it resizes and the key to match the
     * m_BlocksInfo index */
    std::map<size_t, Span> m_BlocksSpan;

    Variable<T>(const std::string &name, const Dims &shape, const Dims &start,
                const Dims &count, const bool constantShape,
                const bool debugMode);

    ~Variable<T>() = default;

    Info &SetBlockInfo(const T *data, const size_t stepsStart,
                       const size_t stepsCount = 1) noexcept;

    void SetData(const T *data) noexcept;

    T *GetData() const noexcept;

    size_t SubStreamsInfoSize();

    Dims Shape(const size_t step = adios2::EngineCurrentStep) const;

    Dims Count() const;

    size_t SelectionSize() const;

    std::pair<T, T> MinMax(const size_t step = adios2::DefaultSizeT) const;

    T Min(const size_t step = adios2::DefaultSizeT) const;

    T Max(const size_t step = adios2::DefaultSizeT) const;

    std::vector<std::vector<typename Variable<T>::Info>>
    AllStepsBlocksInfo() const;

private:
    Dims DoShape(const size_t step) const;

    Dims DoCount() const;

    size_t DoSelectionSize() const;

    std::pair<T, T> DoMinMax(const size_t step) const;

    std::vector<std::vector<typename Variable<T>::Info>>
    DoAllStepsBlocksInfo() const;

    void CheckRandomAccess(const size_t step, const std::string hint) const;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_H_ */
