/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.tcc
 *
 *  Created on: Dec 8, 2017
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_

#include "DataManReader.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void DataManReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    GetDeferredCommon(variable, data);
    PerformGets();
}

template <class T>
void DataManReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    if (m_IsRowMajor)
    {
        while (true)
        {
            int ret = m_FastSerializer.GetVar(
                data, variable.m_Name, variable.m_Start, variable.m_Count,
                m_CurrentStep, variable.m_MemoryStart, variable.m_MemoryCount);
            if (ret == 0)
            {
                break;
            }
        }
    }
    else
    {
        Dims start = variable.m_Start;
        Dims count = variable.m_Count;
        Dims memstart = variable.m_MemoryStart;
        Dims memcount = variable.m_MemoryCount;
        std::reverse(start.begin(), start.end());
        std::reverse(count.begin(), count.end());
        std::reverse(memstart.begin(), memstart.end());
        std::reverse(memcount.begin(), memcount.end());
        while (true)
        {
            int ret =
                m_FastSerializer.GetVar(data, variable.m_Name, start, count,
                                        m_CurrentStep, memstart, memcount);
            if (ret == 0)
            {
                break;
            }
        }
    }
}

template <typename T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
DataManReader::AllStepsBlocksInfoCommon(const Variable<T> &variable) const
{
    return std::map<size_t, std::vector<typename Variable<T>::Info>>();
}

template <typename T>
std::vector<typename Variable<T>::Info>
DataManReader::BlocksInfoCommon(const Variable<T> &variable,
                                const size_t step) const
{
    std::vector<typename Variable<T>::Info> v;
    T max = std::numeric_limits<T>::min();
    T min = std::numeric_limits<T>::max();
    for (const auto &i : *m_CurrentStepMetadata)
    {
        if (i.name == variable.m_Name)
        {
            typename Variable<T>::Info b;
            b.Start = i.start;
            b.Count = i.count;
            b.Shape = i.shape;
            b.IsValue = false;
            if (i.shape.size() == 1)
            {
                if (i.shape[0] == 1)
                {
                    b.IsValue = true;
                }
            }
            AccumulateMinMax(min, max, i.min, i.max);
            v.push_back(b);
        }
    }
    for (auto &i : v)
    {
        i.Min = min;
        i.Max = max;
    }
    return v;
}

template <typename T>
void DataManReader::CheckIOVariable(const std::string &name, const Dims &shape,
                                    const Dims &start, const Dims &count)
{
    bool singleValue = false;
    if (shape.size() == 1 and start.size() == 1 and count.size() == 1)
    {
        if (shape[0] == 1 and start[0] == 0 and count[0] == 1)
        {
            singleValue = true;
        }
    }
    auto v = m_IO.InquireVariable<T>(name);
    if (v == nullptr)
    {
        if (singleValue)
        {
            m_IO.DefineVariable<T>(name);
        }
        else
        {
            m_IO.DefineVariable<T>(name, shape, start, count);
        }
        v = m_IO.InquireVariable<T>(name);
        v->m_Engine = this;
    }
    else
    {
        if (not singleValue)
        {
            if (v->m_Shape != shape)
            {
                v->SetShape(shape);
            }
            if (v->m_Start != start || v->m_Count != count)
            {
                v->SetSelection({start, count});
            }
        }
    }
    v->m_FirstStreamingStep = false;
}

template <>
inline void DataManReader::AccumulateMinMax<std::complex<float>>(
    std::complex<float> &min, std::complex<float> &max,
    const std::vector<char> &minVec, const std::vector<char> &maxVec) const
{
}

template <>
inline void DataManReader::AccumulateMinMax<std::complex<double>>(
    std::complex<double> &min, std::complex<double> &max,
    const std::vector<char> &minVec, const std::vector<char> &maxVec) const
{
}

template <typename T>
void DataManReader::AccumulateMinMax(T &min, T &max,
                                     const std::vector<char> &minVec,
                                     const std::vector<char> &maxVec) const
{
    T maxInMetadata = reinterpret_cast<const T *>(maxVec.data())[0];
    if (maxInMetadata > max)
    {
        max = maxInMetadata;
    }
    T minInMetadata = reinterpret_cast<const T *>(minVec.data())[0];
    if (minInMetadata < min)
    {
        min = minInMetadata;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_ */
