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
Dims Variable<T>::DoShape(const size_t step) const
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
std::pair<T, T> Variable<T>::DoMinMax(const size_t step) const
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

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_TCC_ */
