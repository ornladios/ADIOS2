/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.cpp : needed for template separation using Variable.tcc
 *
 *  Created on: Jun 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Variable.h"
#include "Variable.tcc"

#include "adios2/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h" //helper::GetType<T>

namespace adios2
{
namespace core
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Variable<T>::Variable(const std::string &name, const Dims &shape,          \
                          const Dims &start, const Dims &count,                \
                          const bool constantDims, const bool debugMode)       \
    : VariableBase(name, helper::GetType<T>(), sizeof(T), shape, start, count, \
                   constantDims, debugMode)                                    \
    {                                                                          \
        m_BlocksInfo.reserve(1);                                               \
    }                                                                          \
                                                                               \
    template <>                                                                \
    typename Variable<T>::Info &Variable<T>::SetBlockInfo(                     \
        const T *data, const size_t stepsStart,                                \
        const size_t stepsCount) noexcept                                      \
    {                                                                          \
        Info info;                                                             \
        info.Shape = m_Shape;                                                  \
        info.Start = m_Start;                                                  \
        info.Count = m_Count;                                                  \
        info.BlockID = m_BlockID;                                              \
        info.Selection = m_SelectionType;                                      \
        info.MemoryStart = m_MemoryStart;                                      \
        info.MemoryCount = m_MemoryCount;                                      \
        info.StepsStart = stepsStart;                                          \
        info.StepsCount = stepsCount;                                          \
        info.Data = const_cast<T *>(data);                                     \
        info.Operations = m_Operations;                                        \
        m_BlocksInfo.push_back(info);                                          \
        return m_BlocksInfo.back();                                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::SetData(const T *data) noexcept                          \
    {                                                                          \
        m_Data = const_cast<T *>(data);                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T *Variable<T>::GetData() const noexcept                                   \
    {                                                                          \
        return m_Data;                                                         \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Dims Variable<T>::Shape(const size_t step) const                           \
    {                                                                          \
        return DoShape(step);                                                  \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::pair<T, T> Variable<T>::MinMax(const size_t step) const               \
    {                                                                          \
        return DoMinMax(step);                                                 \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T Variable<T>::Min(const size_t step) const                                \
    {                                                                          \
        return MinMax(step).first;                                             \
    }                                                                          \
                                                                               \
    template <>                                                                \
    T Variable<T>::Max(const size_t step) const                                \
    {                                                                          \
        return MinMax(step).second;                                            \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace core
} // end namespace adios2
