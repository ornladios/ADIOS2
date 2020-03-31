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
#include "adios2/helper/adiosSystem.h"
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SscWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();

    variable.SetData(data);

    Dims vStart = variable.m_Start;
    Dims vCount = variable.m_Count;
    Dims vShape = variable.m_Shape;

    if (vStart.empty() && vCount.empty() && !vShape.empty() &&
        vShape[0] == LocalValueDim)
    {
        // local single value
        throw(std::runtime_error(
            "local single value is not yet supported in SSC"));
    }
    else if (vStart.empty() && !vCount.empty() && vShape.empty())
    {
        // local array
        throw(std::runtime_error("local array is not yet supported in SSC"));
    }
    else if (vStart.empty() && vCount.empty() && vShape.empty())
    {
        // global single value
        throw(std::runtime_error(
            "global single value is not yet supported in SSC"));
    }
    else if (!vStart.empty() && !vCount.empty() && !vShape.empty())
    {
        // global array
        if (!helper::IsRowMajor(m_IO.m_HostLanguage))
        {
            std::reverse(vStart.begin(), vStart.end());
            std::reverse(vCount.begin(), vCount.end());
            std::reverse(vShape.begin(), vShape.end());
        }

        bool found = false;
        for (const auto &b : m_GlobalWritePattern[m_StreamRank])
        {
            if (b.name == variable.m_Name and
                ssc::AreSameDims(vStart, b.start) and
                ssc::AreSameDims(vCount, b.count) and
                ssc::AreSameDims(vShape, b.shape))
            {
                std::memcpy(m_Buffer.data() + b.bufferStart, data,
                            b.bufferCount);
                found = true;
            }
        }

        if (not found)
        {
            if (m_CurrentStep == 0)
            {
                m_GlobalWritePattern[m_StreamRank].emplace_back();
                auto &b = m_GlobalWritePattern[m_StreamRank].back();
                b.name = variable.m_Name;
                b.type = helper::GetType<T>();
                b.shape = vShape;
                b.start = vStart;
                b.count = vCount;
                b.bufferStart = m_Buffer.size();
                b.bufferCount = ssc::TotalDataSize(b.count, b.type);
                m_Buffer.resize(b.bufferStart + b.bufferCount);
                std::memcpy(m_Buffer.data() + b.bufferStart, data,
                            b.bufferCount);
            }
            else
            {
                throw std::runtime_error("ssc only accepts fixed IO pattern");
            }
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCWRITER_TCC_
