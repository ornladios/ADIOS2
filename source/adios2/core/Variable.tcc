/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.tcc : implement long template functions
 *
 *  Created on: Jan 31, 2019
 *      Author: William F Godoy
 */

#ifndef ADIOS2_CORE_VARIABLE_TCC_
#define ADIOS2_CORE_VARIABLE_TCC_

#include "Variable.h"

#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{

template <class T>
Variable<T>::Variable(const std::string &name, const Dims &shape,
                      const Dims &start, const Dims &count,
                      const bool constantDims, const bool debugMode)
: VariableBase(name, helper::GetType<T>(), sizeof(T), shape, start, count,
               constantDims, debugMode)
{
    m_BlocksInfo.reserve(1);
}

template <class T>
typename Variable<T>::Info &
Variable<T>::SetBlockInfo(const T *data, const size_t stepsStart,
                          const size_t stepsCount) noexcept
{
    Info info;
    info.Shape = m_Shape;
    info.Start = m_Start;
    info.Count = m_Count;
    info.BlockID = m_BlockID;
    info.Selection = m_SelectionType;
    info.MemoryStart = m_MemoryStart;
    info.MemoryCount = m_MemoryCount;
    info.StepsStart = stepsStart;
    info.StepsCount = stepsCount;
    info.Data = const_cast<T *>(data);
    info.BufferP = info.Data;
    info.Operations = m_Operations;
    m_BlocksInfo.push_back(info);
    return m_BlocksInfo.back();
}

template <class T>
void Variable<T>::SetData(const T *data) noexcept
{
    m_Data = const_cast<T *>(data);
}

template <class T>
T *Variable<T>::GetData() const noexcept
{
    return m_Data;
}

template <class T>
Dims Variable<T>::Shape(const size_t step) const
{
    if (m_DebugMode)
    {
        if (!m_FirstStreamingStep && step != DefaultSizeT)
        {
            throw std::invalid_argument("ERROR: can't pass a step input in "
                                        "streaming (BeginStep/EndStep)"
                                        "mode for variable " +
                                        m_Name +
                                        ", in call to Variable<T>::Shape\n");
        }
    }

    if (m_FirstStreamingStep && step == DefaultSizeT)
    {
        return m_Shape;
    }

    if (m_Engine != nullptr && m_ShapeID == ShapeID::GlobalArray)
    {
        const size_t stepInput =
            !m_FirstStreamingStep ? m_Engine->CurrentStep() : step;

        const std::vector<typename Variable<T>::Info> blocksInfo =
            m_Engine->BlocksInfo<T>(*this, stepInput);

        if (blocksInfo.size() == 0)
        {
            return Dims();
        }

        if (blocksInfo.front().Shape.size() == 1 &&
            blocksInfo.front().Shape.front() == LocalValueDim)
        {
            return Dims{blocksInfo.size()};
        }

        return blocksInfo.front().Shape;
    }
    return m_Shape;
}

template <class T>
std::pair<T, T> Variable<T>::MinMax(const size_t step) const
{
    if (m_DebugMode && !m_FirstStreamingStep && step != DefaultSizeT)
    {
        throw std::invalid_argument(
            "ERROR: can't pass a step input in streaming (BeginStep/EndStep)"
            "mode for variable " +
            m_Name + ", in call to Variable<T>:: Min Max or MinMax\n");
    }

    std::pair<T, T> minMax;
    minMax.first = {};
    minMax.second = {};

    if (m_Engine != nullptr && !m_FirstStreamingStep)
    {
        const size_t stepInput =
            (step == DefaultSizeT) ? m_Engine->CurrentStep() : step;

        const std::vector<typename Variable<T>::Info> blocksInfo =
            m_Engine->BlocksInfo<T>(*this, stepInput);

        if (blocksInfo.size() == 0)
        {
            return minMax;
        }

        if (m_ShapeID == ShapeID::LocalArray)
        {
            if (m_DebugMode && m_BlockID >= blocksInfo.size())
            {
                throw std::invalid_argument(
                    "ERROR: BlockID " + std::to_string(m_BlockID) +
                    " does not exist for LocalArray variable " + m_Name +
                    ", in call to MinMax, Min or Maxn");
            }
            minMax.first = blocksInfo[m_BlockID].Min;
            minMax.second = blocksInfo[m_BlockID].Max;
            return minMax;
        }

        const bool isValue =
            ((blocksInfo.front().Shape.size() == 1 &&
              blocksInfo.front().Shape.front() == LocalValueDim) ||
             m_ShapeID == ShapeID::GlobalValue)
                ? true
                : false;

        minMax.first =
            isValue ? blocksInfo.front().Value : blocksInfo.front().Min;
        minMax.second =
            isValue ? blocksInfo.front().Value : blocksInfo.front().Max;

        for (const typename Variable<T>::Info &blockInfo : blocksInfo)
        {
            const T minValue = isValue ? blockInfo.Value : blockInfo.Min;

            if (helper::LessThan<T>(minValue, minMax.first))
            {
                minMax.first = minValue;
            }

            const T maxValue = isValue ? blockInfo.Value : blockInfo.Max;

            if (helper::GreaterThan<T>(maxValue, minMax.second))
            {
                minMax.second = maxValue;
            }
        }
    }
    else
    {
        minMax.first = m_Min;
        minMax.second = m_Max;
    }
    return minMax;
}

template <class T>
T Variable<T>::Min(const size_t step) const
{
    return MinMax(step).first;
}

template <class T>
T Variable<T>::Max(const size_t step) const
{
    return MinMax(step).second;
}

template <class T>
std::vector<std::vector<typename Variable<T>::Info>>
Variable<T>::AllStepsBlocksInfo() const
{
    if (m_DebugMode && m_Engine == nullptr)
    {
        if (m_Engine == nullptr)
        {
            throw std::invalid_argument(
                "ERROR: from variable " + m_Name +
                " function is only valid in read mode, in "
                "call to Variable<T>::AllBlocksInfo\n");
        }

        if (!m_FirstStreamingStep)
        {
            throw std::invalid_argument("ERROR: from variable " + m_Name +
                                        " function is not valid in "
                                        "random-access read mode "
                                        "(BeginStep/EndStep), in "
                                        "call to Variable<T>::AllBlocksInfo\n");
        }
    }

    return m_Engine->AllRelativeStepsBlocksInfo(*this);
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_TCC_ */
