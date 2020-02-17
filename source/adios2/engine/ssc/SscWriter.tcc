/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriter.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCWRITER_TCC_
#define ADIOS2_ENGINE_SSCWRITER_TCC_

#include "SscWriter.h"
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
bool SscWriter::HasBlock(const Variable<T> &variable)
{
    for (const auto &b : m_GlobalWritePattern[m_WorldRank])
    {
        if (b.name == variable.m_Name and
            ssc::AreSameDims(variable.m_Start, b.start) and
            ssc::AreSameDims(variable.m_Count, b.count) and
            ssc::AreSameDims(variable.m_Shape, b.shape))
        {
            return true;
        }
    }
    return false;
}

template <class T>
void SscWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();

    if (variable.m_Start.empty())
    {
        variable.m_Start.push_back(0);
    }
    if (variable.m_Count.empty())
    {
        variable.m_Count.push_back(1);
    }
    if (variable.m_Shape.empty())
    {
        variable.m_Shape.push_back(1);
    }

    if (m_CurrentStep == 0)
    {
        if (not HasBlock(variable))
        {
            m_GlobalWritePattern[m_WorldRank].emplace_back();
            auto &b = m_GlobalWritePattern[m_WorldRank].back();
            b.name = variable.m_Name;
            b.type = helper::GetType<T>();
            b.shape = variable.m_Shape;
            b.start = variable.m_Start;
            b.count = variable.m_Count;
            b.bufferStart = m_Buffer.size();
            b.bufferCount = ssc::TotalDataSize(b.count, b.type);
            m_Buffer.resize(b.bufferStart + b.bufferCount);
        }
    }

    variable.SetData(data);

    bool found = false;
    for (const auto &b : m_GlobalWritePattern[m_WorldRank])
    {
        if (b.name == variable.m_Name and
            ssc::AreSameDims(variable.m_Start, b.start) and
            ssc::AreSameDims(variable.m_Count, b.count) and
            ssc::AreSameDims(variable.m_Shape, b.shape))
        {
            std::memcpy(m_Buffer.data() + b.bufferStart, data, b.bufferCount);
            found = true;
        }
    }
    if (not found)
    {
        throw std::runtime_error("ssc only accepts fixed IO pattern");
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCWRITER_TCC_
