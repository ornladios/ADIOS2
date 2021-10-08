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
    CheckRandomAccess(step, "Shape");

    if (m_FirstStreamingStep && step == adios2::EngineCurrentStep)
    {
        return m_Shape;
    }

    if (m_Engine != nullptr && m_ShapeID == ShapeID::GlobalArray)
    {
        const size_t stepInput =
            !m_FirstStreamingStep ? m_Engine->CurrentStep() : step;

        const auto it = m_AvailableShapes.find(stepInput + 1);
        if (it != m_AvailableShapes.end())
        {
            return it->second;
        }
    }
    return m_Shape;
}

template <class T>
Dims Variable<T>::DoCount() const
{
    auto lf_Step = [&]() -> size_t {
        auto itStep =
            std::next(m_AvailableStepBlockIndexOffsets.begin(), m_StepsStart);
        if (itStep == m_AvailableStepBlockIndexOffsets.end())
        {
            auto it = m_AvailableStepBlockIndexOffsets.rbegin();
            throw std::invalid_argument(
                "ERROR: current relative step start for variable " + m_Name +
                " is outside the scope of available steps " +
                std::to_string(it->first - 1) + " in call to Count\n");
        }
        return itStep->first - 1;
    };

    if (m_Engine != nullptr && m_SelectionType == SelectionType::WriteBlock)
    {
        auto MVI = m_Engine->MinBlocksInfo(*this, m_StepsStart);
        if (MVI)
        {
            if (m_BlockID >= MVI->BlocksInfo.size())
            {
                throw std::invalid_argument(
                    "ERROR: blockID " + std::to_string(m_BlockID) +
                    " from SetBlockSelection is out of bounds for available "
                    "blocks size " +
                    std::to_string(MVI->BlocksInfo.size()) + " for variable " +
                    m_Name + " for step " + std::to_string(m_StepsStart) +
                    ", in call to Variable<T>::Count()");
            }

            size_t *DimsPtr = (MVI->BlocksInfo)[m_BlockID].Count;
            Dims D;
            D.resize(MVI->Dims);
            for (int i = 0; i < MVI->Dims; i++)
            {
                D[i] = DimsPtr[i];
            }
            return D;
        }

        const size_t step =
            !m_FirstStreamingStep ? m_Engine->CurrentStep() : lf_Step();

        const std::vector<typename Variable<T>::BPInfo> blocksInfo =
            m_Engine->BlocksInfo<T>(*this, step);

        if (m_BlockID >= blocksInfo.size())
        {
            throw std::invalid_argument(
                "ERROR: blockID " + std::to_string(m_BlockID) +
                " from SetBlockSelection is out of bounds for available "
                "blocks size " +
                std::to_string(blocksInfo.size()) + " for variable " + m_Name +
                " for step " + std::to_string(step) +
                ", in call to Variable<T>::Count()");
        }

        return blocksInfo[m_BlockID].Count;
    }
    return m_Count;
}

template <class T>
size_t Variable<T>::DoSelectionSize() const
{
    return helper::GetTotalSize(DoCount()) * m_StepsCount;
}

template <class T>
std::pair<T, T> Variable<T>::DoMinMax(const size_t step) const
{
    CheckRandomAccess(step, "MinMax");

    std::pair<T, T> minMax;
    minMax.first = {};
    minMax.second = {};

    if (m_Engine != nullptr)
    {

        Engine::MinMaxStruct MM;
        if (m_Engine->VariableMinMax(*this, step, MM))
        {
            minMax.first = MM.MinUnion.Get(minMax.first);
            minMax.second = MM.MaxUnion.Get(minMax.second);
            return minMax;
        }
    }
    if (m_Engine != nullptr && !m_FirstStreamingStep)
    {
        const size_t stepInput =
            (step == DefaultSizeT) ? m_Engine->CurrentStep() : step;

        const std::vector<typename Variable<T>::BPInfo> blocksInfo =
            m_Engine->BlocksInfo<T>(*this, stepInput);

        if (blocksInfo.size() == 0)
        {
            return minMax;
        }

        if (m_ShapeID == ShapeID::LocalArray)
        {
            if (m_BlockID >= blocksInfo.size())
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

        for (const typename Variable<T>::BPInfo &blockInfo : blocksInfo)
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
std::vector<std::vector<typename Variable<T>::BPInfo>>
Variable<T>::DoAllStepsBlocksInfo() const
{
    if (m_Engine == nullptr)
    {
        throw std::invalid_argument("ERROR: from variable " + m_Name +
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

    return m_Engine->AllRelativeStepsBlocksInfo(*this);
}

template <class T>
void Variable<T>::CheckRandomAccess(const size_t step,
                                    const std::string hint) const
{
    if (!m_FirstStreamingStep && step != DefaultSizeT)
    {
        throw std::invalid_argument("ERROR: can't pass a step input in "
                                    "streaming (BeginStep/EndStep)"
                                    "mode for variable " +
                                    m_Name +
                                    ", in call to Variable<T>::" + hint + "\n");
    }
}

// Span functions
template <class T>
Span<T>::Span(Engine &engine, const size_t size)
: m_Engine(engine), m_Size(size)
{
}

template <class T>
size_t Span<T>::Size() const noexcept
{
    return m_Size;
}

template <class T>
T *Span<T>::Data() const noexcept
{
    return m_Engine.BufferData<T>(m_BufferIdx, m_PayloadPosition);
}

template <class T>
T &Span<T>::At(const size_t position)
{
    if (position > m_Size)
    {
        throw std::invalid_argument(
            "ERROR: position " + std::to_string(position) +
            " is out of bounds for span of size " + std::to_string(m_Size) +
            " , in call to T& Span<T>::At\n");
    }

    return (*this)[position];
}

template <class T>
const T &Span<T>::At(const size_t position) const
{
    if (position > m_Size)
    {
        throw std::invalid_argument(
            "ERROR: position " + std::to_string(position) +
            " is out of bounds for span of size " + std::to_string(m_Size) +
            " , in call to const T& Span<T>::At\n");
    }

    return (*this)[position];
}

template <class T>
T &Span<T>::operator[](const size_t position)
{
    T &data = *m_Engine.BufferData<T>(m_BufferIdx,
                                      m_PayloadPosition + position * sizeof(T));
    return data;
}

template <class T>
const T &Span<T>::operator[](const size_t position) const
{
    const T &data = *m_Engine.BufferData<T>(
        m_BufferIdx, m_PayloadPosition + position * sizeof(T));
    return data;
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_TCC_ */
