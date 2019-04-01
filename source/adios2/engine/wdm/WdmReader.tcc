/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WdmReader.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_STAGINGREADER_TCC_
#define ADIOS2_ENGINE_STAGINGREADER_TCC_

#include "WdmReader.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
inline void WdmReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    Log(5, "WdmReader::GetSync(" + variable.m_Name + ") begin", true, true);

    GetDeferredCommon(variable, data);
    PerformGets();

    Log(5, "WdmReader::GetSync(" + variable.m_Name + ") end", true, true);
}

template <class T>
void WdmReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    Log(5, "WdmReader::GetDeferred(" + variable.m_Name + ") begin", true, true);

    if (variable.m_SingleValue)
    {
        variable.m_Shape = Dims(1, 1);
        variable.m_Start = Dims(1, 0);
        variable.m_Count = Dims(1, 1);
    }
    m_DataManSerializer.PutDeferredRequest(variable.m_Name, CurrentStep(),
                                           variable.m_Start, variable.m_Count,
                                           data);

    m_DeferredRequests.emplace_back();
    auto &req = m_DeferredRequests.back();
    req.variable = variable.m_Name;
    req.step = m_CurrentStep;
    req.start = variable.m_Start;
    req.count = variable.m_Count;
    req.data = data;
    req.type = helper::GetType<T>();

    Log(5, "WdmReader::GetDeferred(" + variable.m_Name + ") end", true, true);
}

template <>
inline void WdmReader::AccumulateMinMax<std::complex<float>>(
    std::complex<float> &min, std::complex<float> &max,
    const std::vector<char> &minVec, const std::vector<char> &maxVec) const
{
}

template <>
inline void WdmReader::AccumulateMinMax<std::complex<double>>(
    std::complex<double> &min, std::complex<double> &max,
    const std::vector<char> &minVec, const std::vector<char> &maxVec) const
{
}

template <typename T>
void WdmReader::AccumulateMinMax(T &min, T &max,
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

template <typename T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
WdmReader::AllStepsBlocksInfoCommon(const Variable<T> &variable) const
{
    std::map<size_t, std::vector<typename Variable<T>::Info>> m;
    for (const auto &i : m_MetaDataMap)
    {
        m[i.first] = BlocksInfoCommon(variable, i.first);
    }
    return m;
}

template <typename T>
std::vector<typename Variable<T>::Info>
WdmReader::BlocksInfoCommon(const Variable<T> &variable,
                            const size_t step) const
{
    if (m_Verbosity >= 10)
    {
        std::cout << "WdmReader::BlocksInfoCommon Step " << step << "\n";
    }
    std::vector<typename Variable<T>::Info> v;
    auto it = m_MetaDataMap.find(step);
    if (it == m_MetaDataMap.end())
    {
        return v;
    }
    T max = std::numeric_limits<T>::min();
    T min = std::numeric_limits<T>::max();
    for (const auto &i : *it->second)
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
void WdmReader::CheckIOVariable(const std::string &name, const Dims &shape,
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
        if (m_Verbosity >= 5)
        {
            std::cout << "WdmReader::CheckIOVariable defined Variable" << name
                      << " Dimension " << shape.size() << std::endl;
        }
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
        if (m_Verbosity >= 5)
        {
            std::cout << "WdmReader::CheckIOVariable Variable" << name
                      << " existing, Dimension " << shape.size() << std::endl;
        }
    }
    v->m_FirstStreamingStep = false;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGREADER_TCC_
