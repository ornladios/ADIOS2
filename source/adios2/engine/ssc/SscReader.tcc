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
#include "adios2/helper/adiosFunctions.h"
#include <adios2-perfstubs-interface.h>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SscReader::GetDeferredDeltaCommon(Variable<T> &variable, T *data)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();

    Dims vStart = variable.m_Start;
    Dims vCount = variable.m_Count;
    Dims vShape = variable.m_Shape;

    if (m_IO.m_ArrayOrder != ArrayOrdering::RowMajor)
    {
        std::reverse(vStart.begin(), vStart.end());
        std::reverse(vCount.begin(), vCount.end());
        std::reverse(vShape.begin(), vShape.end());
    }

    m_LocalReadPattern.emplace_back();
    auto &b = m_LocalReadPattern.back();
    b.name = variable.m_Name;
    b.type = helper::GetDataType<T>();
    b.shapeId = variable.m_ShapeID;
    b.start = vStart;
    b.count = vCount;
    b.shape = vShape;
    b.bufferStart = 0;
    b.bufferCount = 0;
    b.data = data;
    b.performed = false;

    for (const auto &d : b.count)
    {
        if (d == 0)
        {
            helper::Throw<std::invalid_argument>(
                "Engine", "SscReader", "GetDeferredDeltaCommon",
                "SetSelection count dimensions cannot be 0");
        }
    }
}

template <>
void SscReader::GetDeferredCommon(Variable<std::string> &variable,
                                  std::string *data)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    variable.SetData(data);

    if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
        m_ReaderSelectionsLocked == false)
    {
        GetDeferredDeltaCommon(variable, data);
    }
    else
    {
        for (const auto &i : m_AllReceivingWriterRanks)
        {
            const auto &v = m_GlobalWritePattern[i.first];
            for (const auto &b : v)
            {
                if (b.name == variable.m_Name)
                {
                    *data = std::string(b.value.begin(), b.value.end());
                }
            }
        }
    }
}

template <class T>
void SscReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    variable.SetData(data);

    Dims vStart = variable.m_Start;
    Dims vCount = variable.m_Count;
    Dims vShape = variable.m_Shape;

    if (m_IO.m_ArrayOrder != ArrayOrdering::RowMajor)
    {
        std::reverse(vStart.begin(), vStart.end());
        std::reverse(vCount.begin(), vCount.end());
        std::reverse(vShape.begin(), vShape.end());
    }

    if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
        m_ReaderSelectionsLocked == false)
    {
        GetDeferredDeltaCommon(variable, data);
    }
    else
    {

        for (const auto &i : m_AllReceivingWriterRanks)
        {
            const auto &v = m_GlobalWritePattern[i.first];
            for (const auto &b : v)
            {
                if (b.name == variable.m_Name)
                {
                    bool empty = false;
                    for (const auto c : b.count)
                    {
                        if (c == 0)
                        {
                            empty = true;
                        }
                    }
                    if (empty)
                    {
                        continue;
                    }

                    if (b.shapeId == ShapeID::GlobalArray ||
                        b.shapeId == ShapeID::LocalArray)
                    {
                        helper::NdCopy(m_Buffer.data<char>() + b.bufferStart,
                                       b.start, b.count, true, true,
                                       reinterpret_cast<char *>(data), vStart,
                                       vCount, true, true, sizeof(T));
                    }
                    else if (b.shapeId == ShapeID::GlobalValue ||
                             b.shapeId == ShapeID::LocalValue)
                    {
                        std::memcpy(data, m_Buffer.data() + b.bufferStart,
                                    b.bufferCount);
                    }
                    else
                    {
                        helper::Log("Engine", "SSCReader", "GetDeferredCommon",
                                    "unknown ShapeID", 0, m_Comm.Rank(), 0,
                                    m_Verbosity, helper::LogMode::ERROR);
                    }
                }
            }
        }
    }
}

template <typename T>
std::vector<typename Variable<T>::BPInfo>
SscReader::BlocksInfoCommon(const Variable<T> &variable,
                            const size_t step) const
{
    PERFSTUBS_SCOPED_TIMER_FUNC();

    std::vector<typename Variable<T>::BPInfo> ret;

    for (const auto &r : m_GlobalWritePattern)
    {
        for (auto &v : r)
        {
            if (v.name == variable.m_Name)
            {
                ret.emplace_back();
                auto &b = ret.back();
                b.Start = v.start;
                b.Count = v.count;
                b.Shape = v.shape;
                b.Step = m_CurrentStep;
                b.StepsStart = m_CurrentStep;
                b.StepsCount = 1;
                if (m_IO.m_ArrayOrder != ArrayOrdering::RowMajor)
                {
                    std::reverse(b.Start.begin(), b.Start.end());
                    std::reverse(b.Count.begin(), b.Count.end());
                    std::reverse(b.Shape.begin(), b.Shape.end());
                }
                if (v.shapeId == ShapeID::GlobalValue ||
                    v.shapeId == ShapeID::LocalValue)
                {
                    b.IsValue = true;
                    if (m_CurrentStep == 0 ||
                        m_WriterDefinitionsLocked == false ||
                        m_ReaderSelectionsLocked == false)
                    {
                        std::memcpy(reinterpret_cast<char *>(&b.Value),
                                    v.value.data(), v.value.size());
                    }
                    else
                    {
                        std::memcpy(reinterpret_cast<char *>(&b.Value),
                                    m_Buffer.data() + v.bufferStart,
                                    v.bufferCount);
                    }
                }
            }
        }
    }
    return ret;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCREADER_TCC_
