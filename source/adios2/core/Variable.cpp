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

#include "adios2/ADIOSMacros.h"
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
    }                                                                          \
                                                                               \
    template <>                                                                \
    typename Variable<T>::Info &Variable<T>::SetStepBlockInfo(                 \
        const T *data, const size_t step) noexcept                             \
    {                                                                          \
        const size_t block = m_StepBlocksInfo[step].size();                    \
        auto &stepBlockInfo = m_StepBlocksInfo[step][block];                   \
        stepBlockInfo.Data = const_cast<T *>(data);                            \
        stepBlockInfo.Shape = m_Shape;                                         \
        stepBlockInfo.Start = m_Start;                                         \
        stepBlockInfo.Count = m_Count;                                         \
        return stepBlockInfo;                                                  \
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
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace core
} // end namespace adios2
