/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReader.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCREADER_TCC_
#define ADIOS2_ENGINE_SSCREADER_TCC_

#include "SscReader.h"
#include "adios2/helper/adiosMemory.h"
#include "adios2/helper/adiosSystem.h"
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
inline void SscReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    TAU_SCOPED_TIMER_FUNC();
    GetDeferredCommon(variable, data);
    PerformGets();
}

template <class T>
void SscReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_CurrentStep == 0)
    {
        m_LocalReadPattern.emplace_back();
        auto &b = m_LocalReadPattern.back();
        b.name = variable.m_Name;
        b.count = variable.m_Count;
        b.start = variable.m_Start;
        b.shape = variable.m_Shape;
        b.type = helper::GetType<T>();

        for (const auto &d : b.count)
        {
            if (d == 0)
            {
                throw(std::runtime_error(
                    "SetSelection count dimensions cannot be 0"));
            }
        }

        m_LocalReadPatternJson["Variables"].emplace_back();
        auto &jref = m_LocalReadPatternJson["Variables"].back();
        jref["Name"] = variable.m_Name;
        jref["Type"] = helper::GetType<T>();
        jref["Start"] = variable.m_Start;
        jref["Count"] = variable.m_Count;
        jref["Shape"] = variable.m_Shape;
        jref["BufferStart"] = 0;
        jref["BufferCount"] = 0;

        ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern);
        ssc::CalculateOverlap(m_GlobalWritePattern, m_LocalReadPattern);
        m_AllReceivingWriterRanks = ssc::AllOverlapRanks(m_GlobalWritePattern);
        CalculatePosition(m_GlobalWritePattern, m_AllReceivingWriterRanks);
        size_t totalDataSize = 0;
        for (auto i : m_AllReceivingWriterRanks)
        {
            totalDataSize += i.second.second;
        }
        m_Buffer.resize(totalDataSize);
        std::vector<MPI_Request> requests;
        for (const auto &i : m_AllReceivingWriterRanks)
        {
            requests.emplace_back();
            MPI_Rget(m_Buffer.data() + i.second.first, i.second.second,
                     MPI_CHAR, i.first, 0, i.second.second, MPI_CHAR, m_MpiWin,
                     &requests.back());
        }
        MPI_Status statuses[requests.size()];
        MPI_Waitall(requests.size(), requests.data(), statuses);
    }

    for (const auto &i : m_AllReceivingWriterRanks)
    {
        const auto &v = m_GlobalWritePattern[i.first];
        for (const auto &b : v)
        {
            if (b.name == variable.m_Name)
            {
                helper::NdCopy<T>(
                    m_Buffer.data() + b.bufferStart, b.start, b.count, true,
                    true, reinterpret_cast<char *>(data), variable.m_Start,
                    variable.m_Count, true, true);
            }
        }
    }
}

template <typename T>
std::map<size_t, std::vector<typename Variable<T>::Info>>
SscReader::AllStepsBlocksInfoCommon(const Variable<T> &variable) const
{
    TAU_SCOPED_TIMER_FUNC();
    std::map<size_t, std::vector<typename Variable<T>::Info>> m;
    return m;
}

template <typename T>
std::vector<typename Variable<T>::Info>
SscReader::BlocksInfoCommon(const Variable<T> &variable,
                            const size_t step) const
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<typename Variable<T>::Info> ret;

    for (const auto &r : m_GlobalWritePattern)
    {
        for (auto &v : r)
        {
            if (v.name != variable.m_Name)
            {
                continue;
            }
            typename Variable<T>::Info b;
            b.Start = v.start;
            b.Count = v.count;
            b.Shape = v.shape;
            b.IsValue = false;
            if (v.shape.size() == 1)
            {
                if (v.shape[0] == 1)
                {
                    b.IsValue = true;
                }
            }
            ret.push_back(b);
        }
    }
    return ret;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCREADER_TCC_
