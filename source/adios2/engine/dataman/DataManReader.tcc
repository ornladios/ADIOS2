/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.tcc
 *
 *  Created on: Dec 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_

#include "DataManReader.h"
#include <iostream>
#include <limits>

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
        while (m_DataManSerializer.GetVar(data, variable.m_Name,
                                          variable.m_Start, variable.m_Count,
                                          m_CurrentStep, variable.m_MemoryStart,
                                          variable.m_MemoryCount) != 0)
        {
        }
    }
    else
    {
        if (m_ContiguousMajor)
        {
            Dims start = variable.m_Start;
            Dims count = variable.m_Count;
            Dims memstart = variable.m_MemoryStart;
            Dims memcount = variable.m_MemoryCount;
            std::reverse(start.begin(), start.end());
            std::reverse(count.begin(), count.end());
            std::reverse(memstart.begin(), memstart.end());
            std::reverse(memcount.begin(), memcount.end());
            while (m_DataManSerializer.GetVar(data, variable.m_Name, start,
                                              count, m_CurrentStep, memstart,
                                              memcount) != 0)
            {
            }
        }
        else
        {
            while (m_DataManSerializer.GetVar(
                       data, variable.m_Name, variable.m_Start,
                       variable.m_Count, m_CurrentStep, variable.m_MemoryStart,
                       variable.m_MemoryCount) != 0)
            {
            }
        }
    }
}

template <typename T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
DataManReader::AllStepsBlocksInfoCommon(const Variable<T> &variable) const
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
DataManReader::BlocksInfoCommon(const Variable<T> &variable,
                                const size_t step) const
{
    std::vector<typename Variable<T>::Info> v;
    auto it = m_MetaDataMap.find(step);
    if (it == m_MetaDataMap.end())
    {
        return v;
    }
    for (const auto &i : *it->second)
    {
        if (i.name == variable.m_Name)
        {
            typename Variable<T>::Info b;
            b.Start = i.start;
            b.Count = i.count;
            b.IsValue = true;
            if (i.count.size() == 1)
            {
                if (i.count[0] == 1)
                {
                    b.IsValue = false;
                }
            }
            // TODO: assign b.Min, b.Max, b.Value
            v.push_back(b);
        }
    }
    return v;
}

template <typename T>
void DataManReader::CheckIOVariable(const std::string &name, const Dims &shape,
                                    const Dims &start, const Dims &count)
{
    auto v = m_IO.InquireVariable<T>(name);
    if (v == nullptr)
    {
        m_IO.DefineVariable<T>(name, shape, start, count);
    }
    else
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

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_ */
