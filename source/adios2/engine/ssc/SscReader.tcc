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

    variable.SetData(data);

    Dims vStart = variable.m_Start;
    Dims vCount = variable.m_Count;
    Dims vShape = variable.m_Shape;

    if (!helper::IsRowMajor(m_IO.m_HostLanguage))
    {
        std::reverse(vStart.begin(), vStart.end());
        std::reverse(vCount.begin(), vCount.end());
        std::reverse(vShape.begin(), vShape.end());
    }

    if (m_CurrentStep == 0)
    {
        m_LocalReadPattern.emplace_back();
        auto &b = m_LocalReadPattern.back();
        b.name = variable.m_Name;
        b.count = vCount;
        b.start = vStart;
        b.shape = vShape;
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
        jref["ShapeID"] = variable.m_ShapeID;
        jref["Start"] = vStart;
        jref["Count"] = vCount;
        jref["Shape"] = vShape;
        jref["BufferStart"] = 0;
        jref["BufferCount"] = 0;

        ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern);
        m_AllReceivingWriterRanks =
            ssc::CalculateOverlap(m_GlobalWritePattern, m_LocalReadPattern);
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
                if (b.shapeId == ShapeID::GlobalArray ||
                    b.shapeId == ShapeID::LocalArray)
                {
                    helper::NdCopy<T>(m_Buffer.data() + b.bufferStart, b.start,
                                      b.count, true, true,
                                      reinterpret_cast<char *>(data), vStart,
                                      vCount, true, true);
                }
                else if (b.shapeId == ShapeID::GlobalValue ||
                         b.shapeId == ShapeID::LocalValue)
                {
                    std::memcpy(data, m_Buffer.data() + b.bufferStart,
                                b.bufferCount);
                }
                else
                {
                    throw(std::runtime_error("ShapeID not supported"));
                }
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
            if (!helper::IsRowMajor(m_IO.m_HostLanguage))
            {
                std::reverse(b.Start.begin(), b.Start.end());
                std::reverse(b.Count.begin(), b.Count.end());
                std::reverse(b.Shape.begin(), b.Shape.end());
            }
            b.IsValue = false;
            if (b.Shape.size() == 1)
            {
                if (b.Shape[0] == 1)
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
