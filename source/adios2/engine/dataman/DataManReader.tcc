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
    variable.SetData(data);
    m_DataManDeserializer.Get(variable, m_CurrentStep);
}

template <class T>
void DataManReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    GetSyncCommon(variable, data);
}

template <typename T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
DataManReader::AllStepsBlocksInfo(const Variable<T> &variable) const
{
    std::map<size_t, std::vector<typename Variable<T>::Info>> m;
    for (const auto &j : m_MetaDataMap)
    {
        std::vector<typename Variable<T>::Info> v;
        for (const auto &i : *j.second)
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
        m[j.first] = v;
    }
    return m;
}

template <typename T>
std::vector<typename Variable<T>::Info>
DataManReader::BlocksInfo(const Variable<T> &variable, const size_t step) const
{
    std::vector<typename Variable<T>::Info> v;
    auto it = m_MetaDataMap.find(step);
    if (it == m_MetaDataMap.end())
    {
        return v;
    }
    for (const auto &i : *it->second)
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
    return v;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_TCC_ */
